"""核对 v2 指纹、Git 基线及只改注释的边界；不写入源码。"""
from pathlib import Path
import hashlib
import json
import re
import subprocess
import zipfile

EV = Path(__file__).resolve().parent
WT = EV.parents[3]
MAIN = WT / '01_固件工程/main'
BASE = '6bd626d'

def sha(data):
    return hashlib.sha256(data).hexdigest()

def git(*args):
    return subprocess.check_output(['git', '-c', 'safe.directory=' + WT.as_posix(), *args], cwd=WT)

def tokens(data):
    # 先处理 C 续行；字符串/字符常量整体保留，注释替换为空白。
    data = re.sub(rb'\\(?:\r\r?\n|\n)', b'', data)
    pattern = rb'/\*.*?\*/|//[^\r\n]*|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27'
    clean = re.sub(pattern, lambda m: b' ' if m[0].startswith((b'/*', b'//')) else m[0], data, flags=re.S)
    return re.findall(rb'"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27|[A-Za-z_][A-Za-z_0-9]*|[0-9][A-Za-z_0-9.]*|>>=|<<=|\.\.\.|->|\+\+|--|<<|>>|<=|>=|==|!=|&&|\|\||\+=|-=|\*=|/=|%=|&=|\^=|\|=|##|[^\s]', clean)

def audit(workspace):
    package = workspace / '05_发布与交付/工程师交接/TW66GW02_工程交接_精简版_v2.zip'
    tracked = git('ls-tree', '-r', '--name-only', '-z', BASE, '--', '01_固件工程/main').decode('utf8').split('\0')
    changes = []
    for name in filter(None, tracked):
        before = git('show', BASE + ':' + name)
        after = (WT / name).read_bytes()
        if before == after:
            continue
        rel = Path(name).relative_to('01_固件工程/main').as_posix()
        assert rel.startswith(('BSP/', 'COP/', 'OS/', 'Core/Src/')) and Path(rel).suffix in {'.c', '.h'}, rel
        assert tokens(before) == tokens(after), 'Non-comment C token change: ' + rel
        encoding = 'gbk' if rel.startswith('OS/') else 'utf8'
        text = after.decode(encoding, errors='strict')
        assert '\ufffd' not in text and re.search('[\u4e00-\u9fff]', text), rel
        old_nl = set(re.findall(rb'\r\r?\n|\n|\r', before))
        assert set(re.findall(rb'\r\r?\n|\n|\r', after)) <= old_nl, rel
        changes.append({'path': rel, 'encoding': encoding, 'before_sha256': sha(before), 'after_sha256': sha(after), 'c_tokens_equal': True})
    count = 0
    with zipfile.ZipFile(package) as z:
        for name in z.namelist():
            if '/01_主程序/main/' in name and not name.endswith('/'):
                rel = name.split('/01_主程序/main/', 1)[1]
                assert z.read(name) == git('show', BASE + ':01_固件工程/main/' + rel), rel
                count += 1
    assert count == 164 and len(changes) == 16, (count, len(changes))
    result = {'result': 'PASS', 'base_commit': git('rev-parse', BASE).decode().strip(),
              'v2_zip_sha256': sha(package.read_bytes()), 'v2_source_files_matched': count,
              'changed_files': changes, 'unchanged_tracked_main_files': len(list(filter(None, tracked))) - len(changes),
              'method': 'C tokens equal; strict original encoding; existing newline forms; all other tracked main files byte equal'}
    (EV / 'comment-audit.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf8')
    print('PASS: v2 source fingerprint 164/164; comments only 16 files; vendor/Bootloader/build tools unchanged', flush=True)
    return result

if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--workspace-root', type=Path, required=True)
    audit(parser.parse_args().workspace_root.resolve())
