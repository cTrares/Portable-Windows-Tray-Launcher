param([string]$Source = (Join-Path $PSScriptRoot 'AppIcon.png'), [string]$Destination = (Join-Path $PSScriptRoot 'app.ico'), [switch]$TrimTransparent)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$sourceImage = [System.Drawing.Image]::FromFile($Source)
$sizes = @(16,20,24,32,48,64,128,256)
$images = @()
try {
  $cropRect = [System.Drawing.RectangleF]::new(0,0,$sourceImage.Width,$sourceImage.Height)
  if ($TrimTransparent) {
    # Ignore the faint outer glow: it wastes most of a tiny notification icon.
    $minX=$sourceImage.Width; $minY=$sourceImage.Height; $maxX=-1; $maxY=-1
    for ($y=0; $y -lt $sourceImage.Height; $y+=4) {
      for ($x=0; $x -lt $sourceImage.Width; $x+=4) {
        if ($sourceImage.GetPixel($x,$y).A -ge 128) {
          $minX=[Math]::Min($minX,$x); $minY=[Math]::Min($minY,$y)
          $maxX=[Math]::Max($maxX,$x); $maxY=[Math]::Max($maxY,$y)
        }
      }
    }
    if ($maxX -lt 0) { throw 'Das Tray-Icon hat keinen sichtbaren Inhalt.' }
    $edge=[Math]::Min([Math]::Max($maxX-$minX,$maxY-$minY)+8,[Math]::Min($sourceImage.Width,$sourceImage.Height))
    $left=[Math]::Clamp(($minX+$maxX-$edge)/2,0,$sourceImage.Width-$edge)
    $top=[Math]::Clamp(($minY+$maxY-$edge)/2,0,$sourceImage.Height-$edge)
    $cropRect=[System.Drawing.RectangleF]::new($left,$top,$edge,$edge)
    Write-Output "Tray-Motiv: $edge statt $($sourceImage.Width) Quellpixel; sichtbares Motiv rund $([Math]::Round(100*($sourceImage.Width/$edge-1))) Prozent groesser."
  }
  foreach ($size in $sizes) {
    $bitmap = [System.Drawing.Bitmap]::new($size,$size)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $ratio = [Math]::Min($size/$cropRect.Width,$size/$cropRect.Height)
    $drawWidth = $cropRect.Width*$ratio
    $drawHeight = $cropRect.Height*$ratio
    $destinationRect = [System.Drawing.RectangleF]::new(($size-$drawWidth)/2,($size-$drawHeight)/2,$drawWidth,$drawHeight)
    $graphics.DrawImage($sourceImage,$destinationRect,$cropRect,[System.Drawing.GraphicsUnit]::Pixel)
    $memory = [System.IO.MemoryStream]::new()
    $bitmap.Save($memory,[System.Drawing.Imaging.ImageFormat]::Png)
    $images += ,($memory.ToArray())
    $memory.Dispose(); $graphics.Dispose(); $bitmap.Dispose()
  }
} finally { $sourceImage.Dispose() }
$writer = [System.IO.BinaryWriter]::new([System.IO.File]::Create($Destination))
try {
  $writer.Write([uint16]0); $writer.Write([uint16]1); $writer.Write([uint16]$sizes.Count)
  $offset = 6 + 16*$sizes.Count
  for ($i=0; $i -lt $sizes.Count; $i++) {
    $dimension = if ($sizes[$i] -eq 256) {0} else {$sizes[$i]}
    $writer.Write([byte]$dimension); $writer.Write([byte]$dimension); $writer.Write([byte]0); $writer.Write([byte]0)
    $writer.Write([uint16]1); $writer.Write([uint16]32); $writer.Write([uint32]$images[$i].Length); $writer.Write([uint32]$offset)
    $offset += $images[$i].Length
  }
  foreach ($bytes in $images) { $writer.Write([byte[]]$bytes) }
} finally { $writer.Dispose() }


