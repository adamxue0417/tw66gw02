# Owned COM objects are released in reverse order; never terminate other Excel instances.
function Initialize-ExcelComTracking {
    $script:ExcelComObjects = [Collections.Generic.List[object]]::new()
}

function Register-ExcelCom($Object) {
    if (($null -ne $Object) -and [Runtime.InteropServices.Marshal]::IsComObject($Object)) {
        $script:ExcelComObjects.Add($Object)
    }
    return $Object
}

function Clear-ExcelComTracking {
    for ($index=$script:ExcelComObjects.Count-1; $index -ge 0; $index--) {
        try { $null=[Runtime.InteropServices.Marshal]::FinalReleaseComObject($script:ExcelComObjects[$index]) }
        catch [Runtime.InteropServices.InvalidComObjectException] { }
    }
    $script:ExcelComObjects.Clear()
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}

function Close-ExcelResources($Application, [object[]]$Books) {
    try {
        foreach ($book in $Books) {
            if ($null -ne $book) {
                try { $book.Close($false) } catch { Write-Warning "Could not close owned workbook: $_" }
            }
        }
    } finally {
        try {
            if ($null -ne $Application) { $Application.Quit() }
        } finally { Clear-ExcelComTracking }
    }
}

function Set-ExcelMatrix($Sheet, [int]$Row, [int]$Column, [object[,]]$Values) {
    if (($Values.GetLength(0) -eq 0) -or ($Values.GetLength(1) -eq 0)) { return }
    $cells=Register-ExcelCom $Sheet.Cells
    $first=Register-ExcelCom ($cells.Item($Row,$Column))
    $last=Register-ExcelCom ($cells.Item($Row+$Values.GetLength(0)-1,$Column+$Values.GetLength(1)-1))
    $range=Register-ExcelCom ($Sheet.Range($first,$last))
    $range.Value2=$Values
}
