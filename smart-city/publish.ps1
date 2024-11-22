# crea un script sencillo que copie los archivos .elf y .hex,
# desde la carpeta C:\Users\andre\AppData\Local\Temp\arduino\sketches\9D03A9692A05D1AB2521D6BBC9D9FF42
# a la carpeta C:\Users\andre\Documents\Arduino\smart-city

$source = "C:\Users\andre\AppData\Local\Temp\arduino\sketches\9D03A9692A05D1AB2521D6BBC9D9FF42"
$destination = "C:\EAFIT\SegundoSemestre\SistemasAutoadaptables\Final\arduino-smart-city-final\smart-city"

if (-Not (Test-Path -Path $destination)) {
    New-Item -ItemType Directory -Path $destination
}

Copy-Item -Path "$source\smart-city.ino.elf" -Destination $destination
Copy-Item -Path "$source\smart-city.ino.hex" -Destination $destination
