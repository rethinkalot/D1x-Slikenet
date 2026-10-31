$p = 'C:\Games\D1s\main\menu.c'
$lines = [IO.File]::ReadAllLines($p)
$changed = 0
for ($i = 0; $i -lt $lines.Length; $i++) {
    $l = $lines[$i]
    $new = $null
    if ($l -match '^\s*m\[(14|28)\]\.type = NM_TYPE_TEXT;\s*$') {
        $n = [int]$Matches[1] + 1
        $new = $l -replace '\[(14|28)\]', "[$n]"
    } elseif ($l -match '^\s*m\[(14|28)\]\.text = "";\s*$') {
        $n = [int]$Matches[1] + 1
        $new = $l -replace '\[(14|28)\]', "[$n]"
    } elseif ($l -match '^\s*ADD_CHECK\((1[4-9]|2[0-6]),') {
        $n = [int]$Matches[1] + 1
        $new = [regex]::Replace($l, 'ADD_CHECK\(\d+,', "ADD_CHECK($n,")
    }
    if ($new -ne $null -and $new -ne $l) { $lines[$i] = $new; $changed++ }
}
[IO.File]::WriteAllLines($p, $lines, (New-Object Text.UTF8Encoding($false)))
"changed: $changed"
