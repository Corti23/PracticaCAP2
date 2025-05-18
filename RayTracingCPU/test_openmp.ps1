# Lista de hilos a probar
$threadsList = @(1, 2, 4, 8, 16)
$outputFile = "resultados_openmp.csv"
$tempOutputFile = "temp_output.txt"

# Modos de paralelización
$modes = @(
    @{ Name = "FILAS";    Defines = @("-DPARALLEL_BY_ROWS=1", "-DPARALLEL_BY_COLUMNS=0", "-DPARALLEL_BY_BLOCKS=0") },
    @{ Name = "COLUMNAS"; Defines = @("-DPARALLEL_BY_ROWS=0", "-DPARALLEL_BY_COLUMNS=1", "-DPARALLEL_BY_BLOCKS=0") },
    @{ Name = "BLOQUES";  Defines = @("-DPARALLEL_BY_ROWS=0", "-DPARALLEL_BY_COLUMNS=0", "-DPARALLEL_BY_BLOCKS=1") }
)

# Cabecera del TSV
"Modo;Threads;Tiempo (segundos)" | Out-File -Encoding utf8 $outputFile

# Loop por cada modo y número de hilos
foreach ($mode in $modes) {
    Write-Output "==================================="
    Write-Output "Compilando para modo: $($mode.Name)"

    # Compilar con los defines del modo actual (se pasan como array de argumentos)
    $defines = $mode.Defines
    g++ -O3 -fopenmp @defines -o raytracing main.cpp utils.cpp random.cpp Scene.cpp Sphere.cpp Metallic.cpp Crystalline.cpp

    foreach ($threads in $threadsList) {
        Write-Output "-------------------------------"
        Write-Output "Ejecutando $($mode.Name) con $threads hilos..."
        $env:OMP_NUM_THREADS = $threads

        # Ejecutar y redirigir salida a temporal
        Start-Process -FilePath ".\raytracing.exe" -NoNewWindow -Wait -RedirectStandardOutput $tempOutputFile

        # Leer la salida del temporal
        $output = Get-Content $tempOutputFile

        # Detectar modo y tiempo
        $modeLine = $output | Where-Object { $_ -like "Paralelización por *" }
        $modeDetected = $modeLine -replace "Paralelización por ", ""

        $timeLine = $output | Select-String -Pattern "Tiempo de renderizado"
        $time = ($timeLine -split ":")[1].Trim().Replace(" segundos.", "")

        # Guardar en el TSV
        "$modeDetected;$threads;$time" | Out-File -Encoding utf8 -Append $outputFile
    }
}

Write-Output "==================================="
Write-Output "¡Todas las pruebas completadas! Resultados guardados en $outputFile."