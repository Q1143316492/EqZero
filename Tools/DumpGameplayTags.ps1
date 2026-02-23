##############################################################################
# DumpGameplayTags.ps1
# Collect all Gameplay Tags from the project and output tree + flat list for AI analysis.
#
# Sources:
#   1. Config/DefaultGameplayTags.ini               (+GameplayTagList=...)
#   2. Plugins/**/Config/Tags/*.ini                 (GameplayTagList=...)
#   3. Source/**/*.cpp / *.h  (UE_DEFINE_GAMEPLAY_TAG / UE_DEFINE_GAMEPLAY_TAG_COMMENT)
#
# Usage:
#   .\Tools\DumpGameplayTags.ps1
#   .\Tools\DumpGameplayTags.ps1 -ProjectRoot "E:\Epic\WorkspaceByRelease\EqZero"
#
# Output:
#   <ProjectRoot>\Tools\GameplayTagDump.txt
##############################################################################

param(
    [string]$ProjectRoot = (Resolve-Path "$PSScriptRoot\..")
)

$OutputFile = Join-Path $ProjectRoot "Tools\GameplayTagDump.txt"

$tags = [System.Collections.Generic.Dictionary[string, hashtable]]::new(
    [System.StringComparer]::OrdinalIgnoreCase
)

function Add-Tag([string]$tag, [string]$comment, [string]$source) {
    $tag = $tag.Trim().Trim('"')
    if ([string]::IsNullOrWhiteSpace($tag)) { return }
    if (-not $tags.ContainsKey($tag)) {
        $tags[$tag] = @{ Comment = $comment.Trim().Trim('"'); Source = $source }
    } elseif ([string]::IsNullOrWhiteSpace($tags[$tag].Comment) -and -not [string]::IsNullOrWhiteSpace($comment)) {
        $tags[$tag].Comment = $comment.Trim().Trim('"')
    }
}

# ---------------------------------------------------------------------------
# 1. Parse INI files
# ---------------------------------------------------------------------------
$iniFiles = [System.Collections.Generic.List[string]]::new()
$defaultIni = Join-Path $ProjectRoot "Config\DefaultGameplayTags.ini"
if (Test-Path $defaultIni) { $iniFiles.Add($defaultIni) }

Get-ChildItem (Join-Path $ProjectRoot "Plugins") -Recurse -Filter "*.ini" -ErrorAction SilentlyContinue |
    Where-Object { $_.DirectoryName -match '\\Tags($|\\)' } |
    ForEach-Object { $iniFiles.Add($_.FullName) }

foreach ($iniFile in $iniFiles) {
    $rel   = $iniFile.Replace($ProjectRoot.ToString(), "").TrimStart("\")
    $lines = Get-Content $iniFile -Encoding UTF8 -ErrorAction SilentlyContinue
    foreach ($line in $lines) {
        if ($line -match 'GameplayTagList=\(Tag="([^"]+)"(?:[^)]*DevComment="([^"]*)")?') {
            Add-Tag $Matches[1] $Matches[2] $rel
        }
    }
}

# ---------------------------------------------------------------------------
# 2. Parse C++ Source
# ---------------------------------------------------------------------------
$cppDirs = @(
    (Join-Path $ProjectRoot "Source"),
    (Join-Path $ProjectRoot "Plugins")
)

foreach ($dir in $cppDirs) {
    if (-not (Test-Path $dir)) { continue }
    Get-ChildItem $dir -Recurse -Include "*.cpp","*.h" -ErrorAction SilentlyContinue | ForEach-Object {
        $rel  = $_.FullName.Replace($ProjectRoot.ToString(), "").TrimStart("\")
        $text = [System.IO.File]::ReadAllText($_.FullName)

        $re1 = [regex]'UE_DEFINE_GAMEPLAY_TAG_COMMENT\s*\(\s*\w+\s*,\s*"([^"]+)"\s*,\s*"([^"]*)"\s*\)'
        foreach ($m in $re1.Matches($text)) {
            Add-Tag $m.Groups[1].Value $m.Groups[2].Value $rel
        }

        $re2 = [regex]'UE_DEFINE_GAMEPLAY_TAG\s*\(\s*\w+\s*,\s*"([^"]+)"\s*\)'
        foreach ($m in $re2.Matches($text)) {
            if (-not $tags.ContainsKey($m.Groups[1].Value)) {
                Add-Tag $m.Groups[1].Value "" $rel
            }
        }
    }
}

# ---------------------------------------------------------------------------
# 3. Build & render tree
# ---------------------------------------------------------------------------
function Build-Tree($allTags) {
    $root = [System.Collections.Generic.SortedDictionary[string, object]]::new(
        [System.StringComparer]::OrdinalIgnoreCase)
    foreach ($tag in ($allTags.Keys | Sort-Object)) {
        $parts = $tag -split '\.'
        $node  = $root
        foreach ($part in $parts) {
            if (-not $node.ContainsKey($part)) {
                $node[$part] = [System.Collections.Generic.SortedDictionary[string, object]]::new(
                    [System.StringComparer]::OrdinalIgnoreCase)
            }
            $node = $node[$part]
        }
    }
    return $root
}

function Render-Tree($node, [string]$prefix, [string]$fullKey, $allTags, [System.Text.StringBuilder]$sb) {
    $children = @($node.Keys)
    for ($i = 0; $i -lt $children.Count; $i++) {
        $key       = $children[$i]
        $isLast    = ($i -eq $children.Count - 1)
        if ($isLast) { $branch = "+-- " } else { $branch = "|-- " }
        if ($fullKey)  { $childKey = "$fullKey.$key" } else { $childKey = $key }
        $childNode = $node[$key]

        $comment = ""
        if ($allTags.ContainsKey($childKey)) {
            $c = $allTags[$childKey].Comment
            if ($c) { $comment = "  // $c" }
        }
        $null = $sb.AppendLine("$prefix$branch$key$comment")

        if ($isLast) { $nextPrefix = $prefix + "    " } else { $nextPrefix = $prefix + "|   " }
        Render-Tree $childNode $nextPrefix $childKey $allTags $sb
    }
}

# ---------------------------------------------------------------------------
# 4. Output
# ---------------------------------------------------------------------------
$sb        = [System.Text.StringBuilder]::new()
$timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
$sep80     = ("=" * 80)

$null = $sb.AppendLine($sep80)
$null = $sb.AppendLine("  EqZero Gameplay Tag Dump")
$null = $sb.AppendLine("  Generated : $timestamp")
$null = $sb.AppendLine("  Total tags: $($tags.Count)")
$null = $sb.AppendLine($sep80)
$null = $sb.AppendLine()

# Tree
$null = $sb.AppendLine(("--- TAG TREE " + ("-" * 67)))
$null = $sb.AppendLine()
$tree = Build-Tree $tags
Render-Tree $tree "" "" $tags $sb
$null = $sb.AppendLine()

# Flat sorted list
$null = $sb.AppendLine(("--- FLAT LIST (alphabetical) " + ("-" * 51)))
$null = $sb.AppendLine()
$maxLen = ($tags.Keys | ForEach-Object { $_.Length } | Measure-Object -Maximum).Maximum
foreach ($tag in ($tags.Keys | Sort-Object)) {
    $info    = $tags[$tag]
    $padded  = $tag.PadRight($maxLen + 2)
    $comment = if ($info.Comment) { "// $($info.Comment)" } else { "" }
    $null = $sb.AppendLine("$padded$comment")
}
$null = $sb.AppendLine()

# Grouped by root namespace
$null = $sb.AppendLine(("--- GROUPED BY ROOT NAMESPACE " + ("-" * 50)))
$null = $sb.AppendLine()
$grouped = $tags.Keys | Sort-Object | Group-Object { ($_ -split '\.')[0] } | Sort-Object Name
foreach ($group in $grouped) {
    $null = $sb.AppendLine("[[ $($group.Name) ]]  ($($group.Count) tags)")
    foreach ($tag in ($group.Group | Sort-Object)) {
        $info    = $tags[$tag]
        $comment = if ($info.Comment) { "  // $($info.Comment)" } else { "" }
        $null = $sb.AppendLine("  $tag$comment")
    }
    $null = $sb.AppendLine()
}

# Source breakdown
$null = $sb.AppendLine(("--- SOURCE FILE BREAKDOWN " + ("-" * 54)))
$null = $sb.AppendLine()
$bySource = $tags.Values | Group-Object { $_.Source } | Sort-Object Count -Descending
foreach ($s in $bySource) {
    $null = $sb.AppendLine("  $($s.Count.ToString().PadLeft(4)) tags  $($s.Name)")
}

# Write UTF-8 without BOM
$outputDir = Split-Path $OutputFile
if (-not (Test-Path $outputDir)) { New-Item $outputDir -ItemType Directory | Out-Null }
$utf8NoBom = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText($OutputFile, $sb.ToString(), $utf8NoBom)

Write-Host ""
Write-Host "Done!  Total tags collected: $($tags.Count)" -ForegroundColor Green
Write-Host "Output -> $OutputFile" -ForegroundColor Cyan
Write-Host ""
