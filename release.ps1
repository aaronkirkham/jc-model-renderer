$version = Get-Content ".\src\version.h" | Select-String "#define VERSION_" | ForEach-Object{$_ -replace '\D+(\d+)', '$1'}

New-Item -ItemType Directory -Force -Path ".\out\Release\bin" | Out-Null

Compress-Archive -Path ".\out\Release\jc-model-renderer.exe" -Force -DestinationPath ".\out\Release\bin\jc-model-renderer-$($version -join ".").zip"
Get-ChildItem ".\out\Release" -Recurse -File -Include *.pdb | Compress-Archive -Force -DestinationPath ".\out\Release\bin\artifacts.zip"