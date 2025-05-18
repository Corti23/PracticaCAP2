# =========================
# Script de automatización
# =========================

# Configuraciones
$procesos = @(1, 2, 4, 8, 16)
$hilos = @(1, 2, 4, 8, 16)
$modos = @(
    @{ Label = "FILAS";   Rows = 1; Columns = 0; Blocks = 0 },
    @{ Label = "COLUMNAS"; Rows = 0; Columns = 1; Blocks = 0 },
    @{ Label = "BLOQUES";  Rows = 0; Columns = 0; Blocks = 1 }
)

# Archivo de resultados
$resultFile = "resultados_openmp_mpi.csv"
# Eliminar si ya existe
Remove-Item -Path $resultFile -ErrorAction SilentlyContinue
# Escribir cabecera
Add-Content -Path $resultFile -Value "Modo;Procesos MPI;Hilos OpenMP;Tiempo (segundos)"

foreach ($modo in $modos) {
    Write-Host "==================================="
    Write-Host "Compilando para modo: $($modo.Label)"
    
    cl /Zi /EHsc /openmp `
        /I"C:\Program Files (x86)\Microsoft SDKs\MPI\Include" `
        main.cpp utils.cpp random.cpp Scene.cpp Sphere.cpp Metallic.cpp Crystalline.cpp `
        /Fe:main.exe `
        /D "PARALLEL_BY_ROWS=$($modo.Rows)" `
        /D "PARALLEL_BY_COLUMNS=$($modo.Columns)" `
        /D "PARALLEL_BY_BLOCKS=$($modo.Blocks)" `
        /link /LIBPATH:"C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64" msmpi.lib

    foreach ($p in $procesos) {
        foreach ($t in $hilos) {
            $env:OMP_NUM_THREADS = $t
            Write-Host "-------------------------------"
            Write-Host "Ejecutando $($modo.Label) con $p procesos MPI y $t hilos OpenMP..."
            
            $output = & mpiexec -n $p main.exe
            $timeLine = $output | Select-String "Tiempo de renderizado" | ForEach-Object { $_.ToString() }
            $timeText = ($timeLine -split ":")[1].Trim().Replace(" segundos.", "").Replace(",", ".")

            # Asegurarse que es numérico con formato invariante
            $time = [double]::Parse($timeText, [System.Globalization.CultureInfo]::InvariantCulture)

            # Guardar resultado correctamente formateado
            $formattedTime = $time.ToString("F6").Replace(".", ",")
            Add-Content -Path $resultFile -Value ("{0};{1};{2};{3}" -f $modo.Label, $p, $t, $formattedTime)

            # if ($time) {
            #     "$($modo.Label);$p;$t;$time;$secuencialTime" | Out-File -Encoding UTF8 -Append $resultFile
            # } else {
            #     Write-Host "No se encontró tiempo en la salida, puede haber fallado la ejecución."
            # }
        }
    }
}

Write-Host "==================================="
Write-Host "¡Pruebas completadas! Resultados guardados en $resultFile."

# # ================================================
# # Script de pruebas automáticas RayTracing OpenMP + MPI
# # Guarda los tiempos en CSV separados por columnas.
# # ================================================

# # Archivo de resultados:
# $resultadosCsv = "resultados_openmp_mpi.csv"
# "Modo;Threads;Tiempo (segundos)" | Out-File -Encoding UTF8 $resultadosCsv

# # Modos de paralelización:
# $modes = @(
#     @{ Name = "FILAS"; Defines = "/D PARALLEL_BY_ROWS=1 /D PARALLEL_BY_COLUMNS=0 /D PARALLEL_BY_BLOCKS=0" },
#     @{ Name = "COLUMNAS"; Defines = "/D PARALLEL_BY_ROWS=0 /D PARALLEL_BY_COLUMNS=1 /D PARALLEL_BY_BLOCKS=0" },
#     @{ Name = "BLOQUES"; Defines = "/D PARALLEL_BY_ROWS=0 /D PARALLEL_BY_COLUMNS=0 /D PARALLEL_BY_BLOCKS=1" }
# )

# # Hilos a probar:
# $threadCounts = @(1, 2, 4, 8, 16)

# # Compilar y ejecutar:
# foreach ($mode in $modes) {
#     Write-Host "==================================="
#     Write-Host "Compilando para modo: $($mode.Name)"

#     $defines = $mode.Defines

#     # Compilación (usando cmd para que acepte bien las rutas y defines):
#     cmd.exe /c "cl /nologo /Zi /EHsc /openmp /I""C:\Program Files (x86)\Microsoft SDKs\MPI\Include"" main.cpp utils.cpp random.cpp Scene.cpp Sphere.cpp Metallic.cpp Crystalline.cpp /Fe:main.exe $defines /link /LIBPATH:""C:\Program Files (x86)\Microsoft SDKs\MPI\Lib\x64"" msmpi.lib"

#     # Ejecución para cada número de hilos:
#     foreach ($threads in $threadCounts) {
#         Write-Host "-------------------------------"
#         Write-Host "Ejecutando $($mode.Name) con $threads hilos..."

#         $env:OMP_NUM_THREADS = $threads
#         $output = & .\main.exe

#         # Buscar el tiempo en la salida:
#         $timeLine = $output | Select-String "Tiempo de renderizado"
#         if ($timeLine) {
#             $time = ($timeLine -split ":")[1].Trim().Replace(" segundos.", "")
#             "$($mode.Name);$threads;$time" | Out-File -Append -Encoding UTF8 $resultadosCsv
#         } else {
#             "$($mode.Name);$threads;ERROR" | Out-File -Append -Encoding UTF8 $resultadosCsv
#         }
#     }
# }

# Write-Host "==================================="
# Write-Host "¡Pruebas completadas! Resultados guardados en $resultadosCsv"