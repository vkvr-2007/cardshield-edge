[CmdletBinding()]
param(
    [ValidateSet("engine", "simulator")]
    [string]$Target = "engine",

    [Parameter(ValueFromRemainingArguments = $true)]
    [string[]]$ProgramArguments
)

$runtimeDirectory = "C:\msys64\ucrt64\bin"
$projectRoot = Split-Path -Parent $PSScriptRoot

if (-not (Test-Path $runtimeDirectory)) {
    throw "Required MSYS2 runtime directory was not found: $runtimeDirectory"
}

$executable = if ($Target -eq "engine") {
    Join-Path $projectRoot "build\edge-engine\edge-engine.exe"
} else {
    Join-Path $projectRoot "build\simulator\cardshield-simulator.exe"
}

if (-not (Test-Path $executable)) {
    throw "Executable was not found: $executable"
}

$env:Path = "$runtimeDirectory;$env:Path"
& $executable @ProgramArguments
exit $LASTEXITCODE
