param(
    [Parameter(Mandatory = $true)]
    [string]$OutputPath,
    [Parameter(Mandatory = $true)]
    [string]$PreviewDir
)

$ErrorActionPreference = 'Stop'

function Get-OleColor([string]$Hex) {
    $h = $Hex.TrimStart('#')
    $r = [Convert]::ToInt32($h.Substring(0, 2), 16)
    $g = [Convert]::ToInt32($h.Substring(2, 2), 16)
    $b = [Convert]::ToInt32($h.Substring(4, 2), 16)
    return $r + ($g * 256) + ($b * 65536)
}

function New-Case {
    param(
        [string]$Id, [string]$Requirement, [string]$Priority, [string]$Item,
        [string]$Precondition, [string]$Steps, [string]$Criteria,
        [string]$Feasibility = '可执行'
    )
    return @($Id, $Requirement, $Priority, $Item, $Precondition, $Steps, $Criteria,
             $Feasibility, '', '', '', '', '')
}

$uiCases = @(
    (New-Case 'UI-A-01' 'a' 'P0' '关机状态按下即开机' '控制器处于完全关机状态。' '按住电源键，不松开；观察按下后的屏幕响应。' '无需松开按键即可触发开机；LED 显示点亮，无反复重启或死机。'),
    (New-Case 'UI-A-02' 'a/f' 'P0' '开机默认 D 表面显示' '主温度板已连接，D 通道有有效常温读数。' '开机后等待温度稳定，记录数字、单位和温度源图标。' '默认显示 D 表面温度；D 图标和当前单位图标点亮；数字可读，显示 LED 亮度正常且无缺段。'),
    (New-Case 'UI-B-01' 'b/h' 'P0' '电源键短按不关机' '设备已开机并显示温度。' '短按电源键，持续时间小于 1 秒后松开。' '设备保持开机；仅切换温度读数来源，不得触发关机。'),
    (New-Case 'UI-B-02' 'b' 'P0' '电源键按住满 1 秒关机' '设备已开机。' '持续按住电源键；用秒表观察到 1 秒时的显示状态，暂不松开。' '按住时间达到 1 秒后立即熄屏并进入关机流程，不需要先松开按键。'),
    (New-Case 'UI-B-03' 'b' 'P0' '任意状态强制关机' '分别使设备处于正常显示、蓝牙配对和 APP 已连接状态。' '每种状态下持续按住电源键至少 1 秒。' '三种状态均能关机；屏幕熄灭，蓝牙连接断开，重新扫描时设备不再保持原连接。'),
    (New-Case 'UI-C-01' 'c' 'P0' '进入蓝牙配对模式' '设备开机，蓝牙当前关闭。' '持续按住蓝牙键至少 1 秒，在触发后松开。' '蓝牙开启并进入可连接状态；蓝牙图标开始闪烁。'),
    (New-Case 'UI-C-02' 'c' 'P1' '配对图标闪烁频率' '设备已进入蓝牙配对模式且尚未连接 APP。' '连续录像或计时 10 秒，统计蓝牙图标完整闪烁周期。' '蓝牙图标约以 1 Hz 闪烁，即约每 0.5 秒亮灭切换一次；10 秒内无明显停顿。'),
    (New-Case 'UI-C-03' 'c' 'P0' '配对 120 秒超时' '设备已进入配对模式，不打开 APP 或主动连接。' '从进入配对模式开始计时 120 秒，观察蓝牙图标和后续可发现状态。' '120 秒内保持配对闪烁；约 120 秒后蓝牙图标熄灭，蓝牙关闭。', '耗时测试'),
    (New-Case 'UI-D-01' 'd' 'P0' '蓝牙连接成功指示' '设备处于配对模式，正式 APP 已打开。' '在 APP 中选择设备并完成连接，观察控制器蓝牙图标。' 'APP 明确显示已连接；控制器蓝牙图标由闪烁转为常亮。'),
    (New-Case 'UI-E-01' 'e' 'P0' '蓝牙键短按切换单位' '设备开机并显示有效温度。' '短按蓝牙键小于 1 秒；重复短按一次。' '首次短按在 °C 与 °F 间切换，数字同步换算；再次短按恢复原单位。'),
    (New-Case 'UI-E-02' 'e' 'P1' '摄氏/华氏图标互斥' '设备显示有效温度。' '分别切换到 °C 和 °F，观察两个单位图标。' '任一时刻仅当前单位图标点亮；切换后读数按 F=round(C×9/5+32) 对应，无两个图标同时点亮。'),
    (New-Case 'UI-E-03' 'e' 'P0' '单位掉电记忆' '先将单位切换为非默认的 °F。' '正常关机，再重新开机；记录单位。随后切回 °C 并重复一次。' '重启后保持关机前选择的单位，读数与单位图标一致。'),
    (New-Case 'UI-F-01' 'f' 'P0' '默认温度来源正确' '三路主温度传感器工作正常且读数可区分。' '开机后不按温度切换键，等待读数稳定。' '显示 D 表面通道及 D 图标，不应默认显示 O、Cavity 或 Probe。'),
    (New-Case 'UI-F-02' 'f' 'P1' '温度读数稳定性' '设备在常温静置，传感器连接稳定。' '连续观察 2 分钟，记录每 10 秒的显示值和图标。' '读数无异常跳变、HI/LO/--- 误报或图标随机切换；变化应符合实际温度趋势。'),
    (New-Case 'UI-G-01' 'g' 'P0' '插入探针自动显示' '设备已开机，探针未插入至少 0.5 秒。' '将食品探针完全插入 3.5 mm 接口，等待读数确认。' '探针被识别后自动切换到 Probe；Probe 图标点亮，其他温度源图标熄灭并显示探针温度。'),
    (New-Case 'UI-G-02' 'g' 'P0' '拔出探针恢复主通道' '当前正在显示 Probe 温度。' '拔出探针并观察显示变化。' 'Probe 图标熄灭；显示从 Probe 返回 D 表面，D 图标点亮，不持续显示旧探针值。'),
    (New-Case 'UI-G-03' 'g/h' 'P1' '未连接探针不出现 Probe' '探针未插入。' '反复短按电源键，完整循环温度来源两轮。' '循环中不出现 Probe 图标或探针读数。'),
    (New-Case 'UI-H-01' 'h' 'P0' '无探针时循环三个主通道' '设备开机，探针未连接，三路主温度有效。' '从默认 D 开始连续短按电源键并记录每一步图标和数值，直到回到 D。' '循环覆盖 D、Cavity、O 三个来源并回到 D；每次仅相应来源图标点亮。'),
    (New-Case 'UI-H-02' 'h' 'P0' '有探针时循环四个通道' '探针已连接且四路温度有效。' '从当前来源连续短按电源键，记录一整轮来源图标。' '一整轮包含 Probe、O、D、Cavity 四个来源，无遗漏或重复卡死；每个读数与对应图标匹配。'),
    (New-Case 'UI-I-01' 'i' 'P1' '主通道温度条阈值' '具备可准确设定主通道温度的模拟器。' '依次施加 29/30、69/70、109/110、149/150、189/190、229/230、269/270、309/310、349/350°C。' '30°C 起点亮最左 1 段，之后每增加 40°C 多亮 1 段；350°C 及以上 9 段全亮且不闪烁。', '需设备'),
    (New-Case 'UI-I-02' 'i' 'P1' '探针温度条阈值' '具备探针温度/电阻模拟器。' '依次施加 9/10、19/20、29/30、39/40、49/50、59/60、69/70、79/80、89/90°C。' '10°C 起点亮最左 1 段，之后每增加 10°C 多亮 1 段；90°C 及以上 9 段全亮且不闪烁。', '需设备'),
    (New-Case 'UI-I-03' 'i' 'P2' '华氏温度条换算阈值' '具备温度模拟器；设备切到 °F。' '主通道依次验证 86、158、230、302、374、446、518、590、662°F；探针验证 50 至 194°F 的九个阈值。' '各段在摄氏阈值换算并四舍五入后的华氏值点亮；段数与对应摄氏测试一致。', '需设备'),
    (New-Case 'UI-J-01' 'j' 'P1' '低电量图标触发' '具备可调电源或可控制的电池模拟输入。' '稳定供电后逐步降至 1.1 V 以下，并保持至少 15 秒观察。' '电压低于 1.1 V 且经过滤波/去抖后，低电量图标点亮；不得在 1.1 V 及以上误亮。', '需设备'),
    (New-Case 'UI-J-02' 'j' 'P1' '低电量恢复迟滞' '低电量图标已点亮；具备可调电源。' '将电压升至 1.2 V 或以上并保持至少 15 秒。' '经过恢复去抖后低电量图标熄灭；1.1–1.2 V 区间内状态不应反复抖动。', '需设备'),
    (New-Case 'UI-K-01' 'k' 'P0' '已连接后断线与重连' 'APP 已连接，蓝牙图标常亮。' '关闭手机蓝牙或强制断开 APP；观察控制器，再恢复蓝牙并由 APP 重连。' '断线后控制器蓝牙图标约 1 Hz 闪烁并持续尝试重连；重连成功后图标恢复常亮。'),
    (New-Case 'UI-L-01' 'l' 'P0' '配对状态长按 1 秒关闭蓝牙' '设备在配对闪烁状态，尚未连接 APP。' '持续按住蓝牙键至少 1 秒。' '蓝牙立即关闭，图标熄灭；无需等待 10 秒。'),
    (New-Case 'UI-L-02' 'l' 'P0' '连接状态按住不足 10 秒不关闭' 'APP 已连接，蓝牙图标常亮。' '按住蓝牙键 1–9 秒后松开。' '连接保持，蓝牙图标保持常亮，不得在 1 秒长按事件时错误关闭。'),
    (New-Case 'UI-L-03' 'l' 'P0' '连接状态按住 10 秒关闭蓝牙' 'APP 已连接，蓝牙图标常亮。' '持续按住蓝牙键，计时达到 10 秒。' '达到 10 秒后蓝牙关闭、图标熄灭，APP 显示断开；设备屏幕其余功能保持正常。'),
    (New-Case 'UI-M-01' 'm' 'P1' '60 分钟无操作自动关机' '设备开机，无 APP 连接；不按键、不插拔探针。' '启动计时并保持 60 分钟完全无交互。' '约 60 分钟后设备自动熄屏并关机，表现与正常关机一致。', '耗时测试'),
    (New-Case 'UI-M-02' 'm' 'P1' '本地交互重置待机计时' '设备已无操作运行约 50 分钟。' '短按一次允许的按键或插拔探针；继续观察至少 15 分钟。' '交互后 10 分钟处不得按原计时自动关机；应从最后一次交互重新计算 60 分钟。', '耗时测试'),
    (New-Case 'UI-N-01' 'n' 'P1' '0°C 与 500°C 边界有效' '具备四通道温度模拟条件。' '对各通道分别施加 0°C 和 500°C，并切换到对应来源。' '0°C 和 500°C 均显示数值，不显示 -LO 或 -HI；图标与通道一致。', '需设备'),
    (New-Case 'UI-N-02' 'n' 'P0' '高于 500°C 显示 -HI' '具备四通道温度模拟条件。' '对 Probe、O、D、Cavity 分别施加高于 500°C 的输入并观察。' '对应通道确认越界后显示 -HI，温度条清空；其他未越界通道不应被错误影响。', '需设备'),
    (New-Case 'UI-N-03' 'n' 'P0' '低于 0°C 显示 -LO' '具备四通道温度模拟条件。' '对 Probe、O、D、Cavity 分别施加低于 0°C 的输入并观察。' '对应通道确认越界后显示 -LO，温度条清空；其他未越界通道不应被错误影响。', '需设备'),
    (New-Case 'UI-O-01' 'o' 'P0' '拆离面板且无探针显示 ---' '设备已开机，探针未连接，可安全拆离 fascia/温度板接口。' '将控制器从 fascia 拆离或断开三路主温度输入。' '三路主温度均不可用时显示 ---，温度源图标和温度条熄灭。'),
    (New-Case 'UI-O-02' 'o' 'P0' '拆离面板时探针继续工作' '先连接探针并确认 Probe 温度，再将控制器拆离 fascia。' '保持探针连接，断开主温度输入并观察。' '仍显示探针温度且 Probe 图标点亮，不应因主温度断开而显示 ---。'),
    (New-Case 'UI-P-01' 'P' 'P1' '三路主温度多项式校正' 'APP 支持系数下发；具备已知温度输入和预先计算的测试系数。' '分别对 Cavity、O/Left、D/Right 下发完整五项系数，并在至少三个输入点读取显示值。' '每个主通道显示 corrected=a·x^4+b·x^3+c·x^2+d·x+e 四舍五入后的结果，通道之间互不串扰。', '需设备'),
    (New-Case 'UI-P-02' 'P' 'P1' '探针不应用主通道校正' '主通道已写入明显非单位系数；探针和主通道输入相同已知温度。' '切换主通道与 Probe 并比较显示。' 'Probe 始终显示探针原始换算温度，不套用 Cavity/O/D 的多项式系数。', '需设备'),
    (New-Case 'UI-P-03' 'P' 'P2' '校正系数掉电保持' '已完成某一主通道的完整系数写入并验证生效。' '正常关机再开机，在相同输入点重新读取该通道。' '重启后仍应用同一组系数；未重新下发时结果与关机前一致。', '需设备')
)

$appCases = @(
    (New-Case 'APP-01' 'BLE基础' 'P0' 'APP 可发现控制器' '控制器已进入蓝牙配对模式；手机蓝牙开启。' '打开正式 APP 的添加/连接设备页面并开始搜索。' 'APP 能在配对有效期内发现目标控制器；列表中不会把同一控制器重复显示为多个设备。'),
    (New-Case 'APP-02' 'c/d' 'P0' 'APP 首次连接成功' 'APP 已发现目标控制器。' '选择控制器并完成 APP 提示的连接流程。' 'APP 进入已连接状态且无持续报错；控制器蓝牙图标由闪烁转为常亮。'),
    (New-Case 'APP-03' 'd' 'P0' '连接状态双向一致' 'APP 已连接。' '同时观察 APP 连接状态和控制器蓝牙图标 30 秒。' 'APP 全程显示已连接，控制器蓝牙图标全程常亮；两端无一端已连、一端未连的不一致。'),
    (New-Case 'APP-04' 'BLE遥测' 'P0' '实时温度自动更新' 'APP 已连接，至少一路温度有效。' '轻微改变可安全改变的探针或主通道温度，观察 APP 2 分钟。' 'APP 无需手动刷新即可持续更新温度；数值变化方向与控制器一致，无长时间冻结。'),
    (New-Case 'APP-05' 'f/g/h' 'P0' '四路通道名称与映射' 'APP 已连接；三路主温度有效，可插入探针。' '记录 Cavity、O/Left、D/Right；插入探针后记录 Probe。用控制器图标和环境温度变化辅助识别。' 'APP 的 Cavity、Left/O、Right/D、Probe 不互换；探针未连接时 Probe 明确显示不可用而非旧值。'),
    (New-Case 'APP-06' 'f/g/h' 'P0' 'APP 与控制器当前温度一致' 'APP 已连接，控制器显示某一路有效温度。' '依次切换 O、D、Cavity 和 Probe；每次稳定后同时截图 APP 对应通道与控制器。' '同一通道在同一时刻显示数值一致；若 APP 固定用 °C，则按相同四舍五入规则换算后数字一致。'),
    (New-Case 'APP-07' 'e/BLE命令' 'P0' 'APP 设置温度单位' 'APP 已连接且提供 °C/°F 设置入口。' '在 APP 中切换到 °F，观察控制器；再切回 °C。' '控制器单位图标和数字随 APP 设置改变；APP 与控制器采用同一显示单位和换算结果。', '需APP支持'),
    (New-Case 'APP-08' 'e/BLE命令' 'P0' 'APP 设置单位掉电保持' '通过 APP 把控制器设置为 °F。' '在 APP 中断开并正常关机；重新开机、重连。' '控制器和 APP 仍为 °F；切回 °C 后再次重启也能保持 °C。', '需APP支持'),
    (New-Case 'APP-09' 'g' 'P0' '探针插拔在 APP 中同步' 'APP 已连接。' '插入探针并等待 APP 更新；记录温度后拔出探针。' '插入后 APP 出现有效 Probe 温度；拔出后 Probe 变为未连接/不可用，不继续显示旧的有效温度。'),
    (New-Case 'APP-10' 'k' 'P0' 'APP 断线和重连' 'APP 已连接。' '关闭手机蓝牙或强制结束 APP，确认断线；重新开启蓝牙和 APP 并重连。' '断线时控制器图标闪烁；重新连接成功后图标常亮，APP 恢复实时温度且无需重启控制器。'),
    (New-Case 'APP-11' 'BLE命令' 'P0' 'APP 远程关机' 'APP 已连接且提供关机命令入口。' '在 APP 中执行设备关机并确认。' '控制器立即熄屏并断开蓝牙；APP 明确显示设备离线，不出现仍连接或仍更新温度。', '需APP支持'),
    (New-Case 'APP-12' 'BLE基础' 'P1' '控制器重启后重新连接' 'APP 曾成功连接过控制器。' '正常关机再开机；重新进入配对并从 APP 连接。' 'APP 能再次找到并连接同一控制器，连接后实时温度恢复。'),
    (New-Case 'APP-13' 'BLE版本' 'P2' 'APP 显示固件版本' 'APP 已连接并提供设备信息页面。' '打开设备信息/诊断页面，记录固件版本。' '固件发布号显示为 100；若 APP 使用其他显示格式，应能明确对应遥测版本字段 100。', '需APP支持'),
    (New-Case 'APP-14' 'BLE恢复出厂' 'P2' 'APP 恢复出厂设置' 'APP 提供恢复出厂入口；先将单位设为 °F。' '执行恢复出厂，等待设备断开；重新进入配对并连接。' '订阅/连接状态被清除并断开；重新连接后单位恢复 °C；控制器身份仍可被识别为同一设备。', '需APP支持'),
    (New-Case 'APP-15' 'BLE单连接' 'P2' '仅允许一台手机连接' '准备两台手机并安装正式 APP。' '手机 A 连接并保持；手机 B 尝试连接同一控制器；随后断开 A 再用 B 连接。' 'A 连接期间 B 不能同时建立有效连接；A 断开后 B 可以连接。', '需设备')
)

$specialCases = @(
    (New-Case 'SP-TEMP-01' 'i' 'P1' '主通道九段温度条完整扫描' '温度模拟器，可独立控制 Cavity/O/D。' '对三路分别扫描 30–350°C 九个阈值及每个阈值前 1°C。' '三路阈值一致；阈值前不提前点亮，达到阈值立即增加一段。', '需设备'),
    (New-Case 'SP-TEMP-02' 'i' 'P1' '探针九段温度条完整扫描' '探针电阻或温度模拟器。' '扫描 10–90°C 九个阈值及每个阈值前 1°C。' '阈值前不提前点亮，达到阈值增加一段，90°C 后九段常亮。', '需设备'),
    (New-Case 'SP-TEMP-03' 'n' 'P0' '四通道上下界和恢复' '可独立设定四路温度。' '逐通道施加 -1/0/500/501°C，再从越界返回有效范围。' '分别显示 -LO/0/500/-HI；恢复有效范围后错误显示清除并恢复数字。', '需设备'),
    (New-Case 'SP-BAT-01' 'j' 'P1' '低电量阈值、去抖及迟滞' '可调电源、万用表。' '测试 1.09/1.10/1.19/1.20 V，记录触发与恢复时间并重复三次。' '仅低于 1.1 V 触发；达到 1.2 V 恢复；状态稳定且重复结果一致。', '需设备'),
    (New-Case 'SP-CAL-01' 'P' 'P1' '三路多项式校正精度' 'APP/协议工具可下发系数，温度模拟器。' '每通道用单位系数、偏移系数和至少一组非线性系数，在三个输入点测试。' '输出与公式计算后四舍五入结果一致，越界校正结果正确转为 HI/LO。', '需设备'),
    (New-Case 'SP-CAL-02' 'P/BLE命令' 'P1' '系数更新原子性和持久化' '协议工具可分别发送 AB/CD/E 三段。' '仅发送一段或两段并等待 60 秒；确认不生效。再发送完整三段并重启。' '不完整更新不改变现用系数；同一通道三段完整后一次性生效；重启后保持。', '需协议工具'),
    (New-Case 'SP-BLE-01' 'BLE广播' 'P1' '广播名称、地址和服务 UUID' 'nRF Connect/等价 BLE 扫描工具。' '抓取广播和扫描响应，检查地址、名称、服务 UUID 和 Model ID。' '地址为稳定 Static Random；广播含 441C1000-776D-B95D-75A7-496DD1B5BEDA；名称符合 mat-XXX；Model ID=0x04。', '需协议工具'),
    (New-Case 'SP-BLE-02' 'BLE GATT/遥测' 'P0' 'FFE1/FFE2 与 1 Hz 遥测' 'BLE 协议工具可写特征和订阅 Notify。' '连接后检查服务；订阅 FFE2，记录连续 10 个通知；向 FFE1 写命令。' 'FFE1 支持 App→设备 Write，FFE2 支持 Notify；通知为 20 字节，约 1 Hz，序号递增。', '需协议工具'),
    (New-Case 'SP-BLE-03' 'BLE遥测' 'P1' '遥测字段、CRC 和错误码' '可保存原始通知并计算 CRC16/CCITT-FALSE。' '解析 Mode、四路 int16、小端电量、版本和错误码；制造探针未连接场景。' 'CRC 正确；温度线传输始终为 °C；无效探针 Mode bit 清零且值 4000；版本=100；保留位为 0。', '需协议工具'),
    (New-Case 'SP-BLE-04' 'BLE命令' 'P1' '非法帧不改变设备状态' '协议工具可构造原始 Mathis 帧。' '分别发送错误 CRC、错误长度、错误 Model ID、非法通道和 NaN 系数。' '设备不执行命令、不改变单位/系数/电源状态，随后合法帧仍可正常处理。', '需协议工具'),
    (New-Case 'SP-BLE-05' 'BLE安全' 'P2' '配对安全与无绑定重连' '手机可查看配对方式和系统蓝牙记录。' '首次连接观察配对方式；断开后重新连接并检查是否复用长期绑定。' '模块支持时采用 Just Works LE Secure Connections；不要求 PIN；正常重连不依赖长期绑定记录。', '需协议工具')
)

$headers = @('用例编号','需求编号','优先级','测试项目','前置条件','详细步骤','通过标准','本轮可执行性','实际结果','结果','证据/照片编号','问题说明','复测结果')
$columnWidths = @(14,11,8,24,34,58,58,15,32,11,18,30,12)
$palette = @{
    Navy = Get-OleColor '#17324D'; Teal = Get-OleColor '#157A75'; Blue = Get-OleColor '#2E5E8C';
    LightBlue = Get-OleColor '#EAF2F8'; LightTeal = Get-OleColor '#E8F5F3'; White = Get-OleColor '#FFFFFF';
    Text = Get-OleColor '#203040'; Border = Get-OleColor '#D5DEE5'; Green = Get-OleColor '#D9EAD3';
    Red = Get-OleColor '#F4CCCC'; Yellow = Get-OleColor '#FFF2CC'; Gray = Get-OleColor '#E7E6E6';
    Purple = Get-OleColor '#E4DFEC'; Orange = Get-OleColor '#FCE4D6'
}

function Set-TitleBand($sheet, [string]$title, [string]$subtitle, [int]$lastColumn) {
    $sheet.Cells.Clear()
    $sheet.Range($sheet.Cells.Item(1,1), $sheet.Cells.Item(1,$lastColumn)).Merge()
    $sheet.Cells.Item(1,1).Value2 = $title
    $sheet.Cells.Item(1,1).Interior.Color = $palette.Navy
    $sheet.Cells.Item(1,1).Font.Color = $palette.White
    $sheet.Cells.Item(1,1).Font.Bold = $true
    $sheet.Cells.Item(1,1).Font.Size = 18
    $sheet.Cells.Item(1,1).HorizontalAlignment = -4108
    $sheet.Rows.Item(1).RowHeight = 32
    $sheet.Range($sheet.Cells.Item(2,1), $sheet.Cells.Item(2,$lastColumn)).Merge()
    $sheet.Cells.Item(2,1).Value2 = $subtitle
    $sheet.Cells.Item(2,1).Interior.Color = $palette.LightBlue
    $sheet.Cells.Item(2,1).Font.Color = $palette.Text
    $sheet.Cells.Item(2,1).Font.Size = 10
    $sheet.Cells.Item(2,1).WrapText = $true
    $sheet.Cells.Item(2,1).HorizontalAlignment = -4131
    $sheet.Rows.Item(2).RowHeight = 34
}

function Add-StatusFormatting($range) {
    $range.FormatConditions.Delete()
    $items = @(
        @('通过', $palette.Green), @('失败', $palette.Red), @('阻塞', $palette.Yellow),
        @('不适用', $palette.Purple)
    )
    foreach ($item in $items) {
        $condition = $range.FormatConditions.Add(1, 3, ('="' + $item[0] + '"'))
        $condition.Interior.Color = $item[1]
        $condition.Font.Bold = $true
    }
}

function Add-TestSheet($workbook, [string]$name, [string]$title, [string]$subtitle, $rows, [string]$tableName) {
    $sheet = $workbook.Worksheets.Add()
    $sheet.Name = $name
    Set-TitleBand $sheet $title $subtitle 13

    for ($c = 0; $c -lt $headers.Count; $c++) {
        $sheet.Cells.Item(4, $c + 1).Value2 = $headers[$c]
        $sheet.Columns.Item($c + 1).ColumnWidth = $columnWidths[$c]
    }
    $headerRange = $sheet.Range('A4:M4')
    $headerRange.Interior.Color = $palette.Teal
    $headerRange.Font.Color = $palette.White
    $headerRange.Font.Bold = $true
    $headerRange.HorizontalAlignment = -4108
    $headerRange.VerticalAlignment = -4108
    $headerRange.WrapText = $true
    $sheet.Rows.Item(4).RowHeight = 32

    $startRow = 5
    for ($r = 0; $r -lt $rows.Count; $r++) {
        for ($c = 0; $c -lt 13; $c++) {
            $sheet.Cells.Item($startRow + $r, $c + 1).Value2 = $rows[$r][$c]
        }
    }
    $endRow = $startRow + $rows.Count - 1
    $dataRange = $sheet.Range("A4:M$endRow")
    $dataRange.Font.Name = 'Microsoft YaHei'
    $dataRange.Font.Size = 10
    $dataRange.VerticalAlignment = -4160
    $dataRange.Borders.Color = $palette.Border
    $dataRange.Borders.Weight = 2
    $sheet.Range("D5:M$endRow").WrapText = $true
    $sheet.Range("A5:C$endRow").HorizontalAlignment = -4108
    $sheet.Range("H5:H$endRow").HorizontalAlignment = -4108
    $sheet.Range("J5:J$endRow").HorizontalAlignment = -4108
    $sheet.Range("M5:M$endRow").HorizontalAlignment = -4108
    $sheet.Range("A5:M$endRow").RowHeight = 68

    $table = $sheet.ListObjects.Add(1, $dataRange, $null, 1)
    $table.Name = $tableName
    $table.TableStyle = 'TableStyleMedium2'
    $table.ShowAutoFilter = $true

    $statusRange = $sheet.Range("J5:J$endRow")
    $retestRange = $sheet.Range("M5:M$endRow")
    $statusRange.Validation.Delete()
    $retestRange.Validation.Delete()
    $statusRange.Validation.Add(3, 1, 1, "='验证列表'!`$A`$1:`$A`$4")
    $retestRange.Validation.Add(3, 1, 1, "='验证列表'!`$A`$1:`$A`$4")
    $sheet.Range("C5:C$endRow").Validation.Delete()
    $sheet.Range("C5:C$endRow").Validation.Add(3, 1, 1, "='验证列表'!`$B`$1:`$B`$3")
    $sheet.Range("H5:H$endRow").Validation.Delete()
    $sheet.Range("H5:H$endRow").Validation.Add(3, 1, 1, "='验证列表'!`$C`$1:`$C`$5")
    Add-StatusFormatting $statusRange
    Add-StatusFormatting $retestRange

    $sheet.Activate()
    $sheet.Application.ActiveWindow.SplitRow = 4
    $sheet.Application.ActiveWindow.SplitColumn = 3
    $sheet.Application.ActiveWindow.FreezePanes = $true
    $sheet.Application.ActiveWindow.DisplayGridlines = $false
    $sheet.PageSetup.Orientation = 2
    $sheet.PageSetup.Zoom = $false
    $sheet.PageSetup.FitToPagesWide = 1
    $sheet.PageSetup.FitToPagesTall = $false
    $sheet.PageSetup.PrintTitleRows = '$1:$4'
    return $sheet
}

$outputDir = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
New-Item -ItemType Directory -Force -Path $PreviewDir | Out-Null

$excel = $null
$workbook = $null
try {
    $excel = New-Object -ComObject Excel.Application
    $excel.Visible = $false
    $excel.DisplayAlerts = $false
    $excel.ScreenUpdating = $false
    $workbook = $excel.Workbooks.Add()

    while ($workbook.Worksheets.Count -gt 1) {
        $workbook.Worksheets.Item($workbook.Worksheets.Count).Delete()
    }

    $validation = $workbook.Worksheets.Item(1)
    $validation.Name = '验证列表'
    @('通过','失败','阻塞','不适用') | ForEach-Object -Begin {$i=1} -Process {$validation.Cells.Item($i,1).Value2=$_; $i++}
    @('P0','P1','P2') | ForEach-Object -Begin {$i=1} -Process {$validation.Cells.Item($i,2).Value2=$_; $i++}
    @('可执行','耗时测试','需设备','需APP支持','需协议工具') | ForEach-Object -Begin {$i=1} -Process {$validation.Cells.Item($i,3).Value2=$_; $i++}

    # Optional feasibility text in the source rows is concatenated into the
    # criteria cell by Windows PowerShell. Exclude every row whose criteria
    # explicitly says it needs extra equipment, protocol tools, or APP support.
    $uiVisibleCases = @($uiCases | Where-Object { $_[6] -notmatch '需(设备|协议工具|APP支持)' })
    $appVisibleCases = @($appCases | Where-Object {
        ($_[6] -notmatch '需(设备|协议工具|APP支持)') -and
        ($_[0] -notin @('APP-13','APP-14','APP-15'))
    })
    $uiSheet = Add-TestSheet $workbook 'UI操作逻辑' 'Mathis UI 操作逻辑测试' '依据 260323 Mathis UI Operational Logic V1.6；仅保留当前正式 APP 与黑盒实机可执行项目。' $uiVisibleCases 'UiLogicTests'
    $appSheet = Add-TestSheet $workbook 'APP基础测试' 'Mathis 正式 APP 基础测试' '仅使用正式 APP 和用户可见现象；覆盖发现、连接、实时数据、通道映射、探针同步和重连，不包含 OTA。' $appVisibleCases 'AppSmokeTests'

    $overview = $workbook.Worksheets.Add()
    $overview.Name = '测试概览'
    Set-TitleBand $overview 'Mathis 控制器黑盒测试清单' '范围：当前可执行的 UI Operational Logic V1.6 + 正式 APP 基础连接测试；明确排除 OTA。' 10
    $overview.Activate()
    $overview.Application.ActiveWindow.DisplayGridlines = $false
    $overview.Application.ActiveWindow.SplitRow = 3
    $overview.Application.ActiveWindow.FreezePanes = $true
    $overview.Columns.Item(1).ColumnWidth = 4
    $overview.Columns.Item(2).ColumnWidth = 24
    $overview.Columns.Item(3).ColumnWidth = 50
    $overview.Columns.Item(4).ColumnWidth = 4
    $overview.Columns.Item(5).ColumnWidth = 18
    $overview.Columns.Item(6).ColumnWidth = 18
    $overview.Columns.Item(7).ColumnWidth = 18
    $overview.Columns.Item(8).ColumnWidth = 18
    $overview.Columns.Item(9).ColumnWidth = 18
    $overview.Columns.Item(10).ColumnWidth = 18

    $overview.Range('B4:C4').Merge()
    $overview.Range('B4').Value2 = '测试环境（执行前填写空白项）'
    $overview.Range('B4:C4').Interior.Color = $palette.Teal
    $overview.Range('B4:C4').Font.Color = $palette.White
    $overview.Range('B4:C4').Font.Bold = $true
    $envRows = @(
        @('控制器编号 / SN',''),
        @('固件文件','tw66gw02.hex'),
        @('固件发布号预期','100'),
        @('固件 SHA-256','D9AEA8C41E751A439D266DFC0042B92858E22E7DAF0C3899D92C6A237C5AAD63'),
        @('正式 APP 版本',''),
        @('手机型号',''),
        @('手机系统版本',''),
        @('测试人员',''),
        @('测试日期',''),
        @('测试条件','正式 APP；仅黑盒实机')
    )
    for ($r=0; $r -lt $envRows.Count; $r++) {
        $overview.Cells.Item(5+$r,2).Value2 = $envRows[$r][0]
        $overview.Cells.Item(5+$r,3).Value2 = $envRows[$r][1]
    }
    $overview.Range('B5:B14').Interior.Color = $palette.LightBlue
    $overview.Range('B5:B14').Font.Bold = $true
    $overview.Range('B5:C14').Borders.Color = $palette.Border
    $overview.Range('B5:C14').Borders.Weight = 2
    $overview.Range('B5:C14').WrapText = $true
    $overview.Rows('5:14').RowHeight = 30

    $overview.Range('E4:J4').Merge()
    $overview.Range('E4').Value2 = '执行结果汇总'
    $overview.Range('E4:J4').Interior.Color = $palette.Blue
    $overview.Range('E4:J4').Font.Color = $palette.White
    $overview.Range('E4:J4').Font.Bold = $true
    $summaryLabels = @('用例总数','通过','失败','阻塞','不适用')
    for ($c=0; $c -lt $summaryLabels.Count; $c++) {
        $overview.Cells.Item(5,5+$c).Value2 = $summaryLabels[$c]
        $overview.Cells.Item(5,5+$c).Interior.Color = $palette.LightTeal
        $overview.Cells.Item(5,5+$c).Font.Bold = $true
        $overview.Cells.Item(5,5+$c).HorizontalAlignment = -4108
    }
    $overview.Cells.Item(6,5).Formula = "=COUNTA('UI操作逻辑'!A5:A200)+COUNTA('APP基础测试'!A5:A200)"
    $statuses = @('通过','失败','阻塞','不适用')
    for ($c=0; $c -lt $statuses.Count; $c++) {
        $col = 6+$c
        $s = $statuses[$c]
        $overview.Cells.Item(6,$col).Formula = "=COUNTIF('UI操作逻辑'!J5:J200,`"$s`")+COUNTIF('APP基础测试'!J5:J200,`"$s`")"
    }
    $overview.Range('E5:I6').Borders.Color = $palette.Border
    $overview.Range('E5:I6').Borders.Weight = 2
    $overview.Range('E6:I6').Font.Bold = $true
    $overview.Range('E6:I6').Font.Size = 16
    $overview.Range('E6:I6').HorizontalAlignment = -4108
    $overview.Cells.Item(6,6).Interior.Color = $palette.Green
    $overview.Cells.Item(6,7).Interior.Color = $palette.Red
    $overview.Cells.Item(6,8).Interior.Color = $palette.Yellow
    $overview.Cells.Item(6,9).Interior.Color = $palette.Purple

    $overview.Range('E8:F8').Merge()
    $overview.Range('E8').Value2 = '有效通过率'
    $overview.Range('E8:F8').Interior.Color = $palette.Teal
    $overview.Range('E8:F8').Font.Color = $palette.White
    $overview.Range('E8:F8').Font.Bold = $true
    $overview.Range('G8:J9').Merge()
    $overview.Range('G8').Formula = '=IF((F6+G6)=0,0,F6/(F6+G6))'
    $overview.Range('G8').NumberFormat = '0.0%'
    $overview.Range('G8').Font.Size = 24
    $overview.Range('G8').Font.Bold = $true
    $overview.Range('G8').HorizontalAlignment = -4108
    $overview.Range('G8').VerticalAlignment = -4108
    $overview.Range('E8:J9').Borders.Color = $palette.Border
    $overview.Range('E8:J9').Borders.Weight = 2

    $overview.Range('B17:J17').Merge()
    $overview.Range('B17').Value2 = '使用说明与判定规则'
    $overview.Range('B17:J17').Interior.Color = $palette.Teal
    $overview.Range('B17:J17').Font.Color = $palette.White
    $overview.Range('B17:J17').Font.Bold = $true
    $notes = @(
        '1. 先填写控制器编号、APP 版本、手机型号和测试日期；确认实机烧录文件与本页固件基线一致。',
        '2. 每个用例执行后填写“实际结果”，从“结果”下拉框选择：通过 / 失败 / 阻塞 / 不适用；未执行时保持空白。',
        '3. 本表仅包含当前能够使用黑盒实机和正式 APP 执行的项目。',
        '4. 失败项必须填写问题说明和证据/照片编号；修复后在“复测结果”选择最终结论。',
        '5. 有效通过率只按 已通过 /（已通过 + 已失败）计算，不把阻塞、空白和不适用计入分母。',
        '6. OTA 明确不在本工作簿范围内。'
    )
    for ($r=0; $r -lt $notes.Count; $r++) {
        $overview.Range($overview.Cells.Item(18+$r,2),$overview.Cells.Item(18+$r,10)).Merge()
        $overview.Cells.Item(18+$r,2).Value2 = $notes[$r]
        $overview.Cells.Item(18+$r,2).WrapText = $true
        $noteFill = if ($r % 2 -eq 0) {$palette.LightBlue} else {$palette.White}
        $overview.Cells.Item(18+$r,2).Interior.Color = $noteFill
        $overview.Rows.Item(18+$r).RowHeight = 28
    }
    $overview.Range('B18:J23').Borders.Color = $palette.Border
    $overview.Range('B18:J23').Borders.Weight = 2
    $overview.Range('B1:J23').Font.Name = 'Microsoft YaHei'
    $overview.PageSetup.Orientation = 2
    $overview.PageSetup.Zoom = $false
    $overview.PageSetup.FitToPagesWide = 1
    $overview.PageSetup.FitToPagesTall = 1

    $overview.Move($workbook.Worksheets.Item(1))
    $validation.Visible = 2
    $overview.Activate()
    $workbook.SaveAs($OutputPath, 51)
    $workbook.Close($true)
    $workbook = $null

    # Reopen read-only and export one PDF preview per visible sheet for visual QA.
    $checkBook = $excel.Workbooks.Open($OutputPath, 0, $true)
    foreach ($sheetName in @('测试概览','UI操作逻辑','APP基础测试')) {
        $sheet = $checkBook.Worksheets.Item($sheetName)
        $sheet.ExportAsFixedFormat(0, (Join-Path $PreviewDir ($sheetName + '.pdf')), 0, $true, $false)
    }
    $checkBook.Close($false)
    $excel.Quit()
    $excel = $null
}
finally {
    if ($workbook -ne $null) { $workbook.Close($false) }
    if ($excel -ne $null) { $excel.Quit() }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
