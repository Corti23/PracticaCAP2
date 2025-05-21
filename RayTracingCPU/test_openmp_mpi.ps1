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

            if ($p -eq 4 -and $t -eq 8 -and $modo.Label -eq "FILAS") {
                # Llamada especial para generar imágenes
                & mpiexec -n $p main.exe render
            }
            
            $output = & mpiexec -n $p main.exe
            $timeLine = $output | Select-String "Tiempo de renderizado" | ForEach-Object { $_.ToString() }
            $timeText = ($timeLine -split ":")[1].Trim().Replace(" segundos.", "").Replace(",", ".")

            # Asegurarse que es numérico con formato invariante
            $time = [double]::Parse($timeText, [System.Globalization.CultureInfo]::InvariantCulture)

            # Guardar resultado correctamente formateado
            $formattedTime = $time.ToString("F6").Replace(".", ",")
            Add-Content -Path $resultFile -Value ("{0};{1};{2};{3}" -f $modo.Label, $p, $t, $formattedTime)
        }
    }
}

Write-Host "==================================="
Write-Host "¡Pruebas completadas! Resultados guardados en $resultFile."