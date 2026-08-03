param(
    [Parameter(Position=0)]
    [string]$HostAddr = "samp.ulgaming.net",

    [Parameter(Position=1)]
    [int]$Port = 7777,

    [Parameter(Position=2, ValueFromRemainingArguments=$true)]
    [string[]]$Nicks = @("Bot_1", "Bot_2", "Bot_3"),

    [string]$Password = "",
    [int]$MinAmpl = 50,
    [int]$MaxAmpl = 250,
    [int]$Ampl = -1
)

$ExePath = Join-Path $PSScriptRoot "client\bin\RakSAMPClient.exe"

if (-not (Test-Path $ExePath))
{
    Write-Error "Could not find RakSAMPClient.exe at $ExePath"
    exit 1
}

Write-Host "Launching $($Nicks.Count) bot(s) to ${HostAddr}:${Port}..." -ForegroundColor Green

foreach ($Nick in $Nicks)
{
    $InstanceAmpl = if ($Ampl -ge 0) { $Ampl } else { Get-Random -Minimum $MinAmpl -Maximum ($MaxAmpl + 1) }

    $Args = "-n $Nick -h $HostAddr -p $Port -ampl $InstanceAmpl"
    if ($Password -ne "")
    {
        $Args += " -pass $Password"
    }

    Write-Host "  -> Launching $Nick with +${InstanceAmpl}ms amplification" -ForegroundColor Yellow
    Start-Process -FilePath $ExePath -ArgumentList $Args
    Start-Sleep -Milliseconds 250
}

Write-Host "Done! Launched $($Nicks.Count) bot processes." -ForegroundColor Cyan
