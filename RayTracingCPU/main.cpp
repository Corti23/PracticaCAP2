#include <mpi.h>
#include <float.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <limits>
#include <sstream>
#include <fstream>
#include <omp.h>

#include "Camera.h"
#include "Object.h"
#include "Scene.h"
#include "Sphere.h"
#include "Diffuse.h"
#include "Metallic.h"
#include "Crystalline.h"

#include "random.h"
#include "utils.h"

// ==============================
// Selección de paralelización:
// ==============================
#ifndef PARALLEL_BY_ROWS
	#define PARALLEL_BY_ROWS 1  // Valor por defecto si no se define por compilador
#endif

#ifndef PARALLEL_BY_COLUMNS
	#define PARALLEL_BY_COLUMNS 0
#endif

#ifndef PARALLEL_BY_BLOCKS
	#define PARALLEL_BY_BLOCKS 0
#endif


// Asegurarse de que solo haya uno activado:
#if (PARALLEL_BY_ROWS + PARALLEL_BY_COLUMNS + PARALLEL_BY_BLOCKS) != 1
    #error "¡Debes activar SOLO UNA estrategia de paralelización!"
#endif

// Mostrar la estrategia activa:
void printParallelStrategy() {
	#if PARALLEL_BY_ROWS
		printf("Paralelización por FILAS (j)\n");
	#elif PARALLEL_BY_COLUMNS
		printf("Paralelización por COLUMNAS (i)\n");
	#elif PARALLEL_BY_BLOCKS
		printf("Paralelización por BLOQUES RECTANGULARES\n");
	#endif
}

// Configuración para los bloques rectangulares:
int block_count_x = 4;  // Número de bloques en horizontal (X)
int block_count_y = 4;  // Número de bloques en vertical (Y)

Scene loadObjectsFromFile(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;

	Scene list;

	if (file.is_open()) {
		while (std::getline(file, line)) {
			std::stringstream ss(line);
			std::string token;
			std::vector<std::string> tokens;

			while (ss >> token) {
				tokens.push_back(token);
			}

			if (tokens.empty()) continue; // Linea vacia

			// Esperamos al menos la palabra clave "Object"
			if (tokens[0] == "Object" && tokens.size() >= 12) { // Minimo para Sphere y un material con 1 float
				// Parsear la esfera
				if (tokens[1] == "Sphere" && tokens[2] == "(" && tokens[7] == ")") {
					try {
						float sx = std::stof(tokens[3].substr(tokens[3].find('(') + 1, tokens[3].find(',') - tokens[3].find('(') - 1));
						float sy = std::stof(tokens[4].substr(0, tokens[4].find(',')));
						float sz = std::stof(tokens[5].substr(0, tokens[5].find(',')));
						float sr = std::stof(tokens[6]);

						// Parsear el material del ultimo objeto creado

						if (tokens[8] == "Crystalline" && tokens[9] == "(" && tokens[11].back() == ')') {
							float ma = std::stof(tokens[10]);

							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Crystalline(ma)
							));

							std::cout << "Crystaline " << sx << " " << sy << " " << sz << " " << sr << " " << ma << "\n";
						} else if (tokens[8] == "Metallic" && tokens.size() == 15 && tokens[9] == "(" && tokens[14] == ")") {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));
							float mf = std::stof(tokens[13].substr(0, tokens[13].length() - 1));

							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Metallic(Vec3(ma, mb, mc), mf)
							));

							std::cout << "Metallic " << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << " " << mf << "\n";
						} else if (tokens[8] == "Diffuse" && tokens.size() == 14 && tokens[9] == "(" && tokens[13].back() == ')') {
							float ma = std::stof(tokens[10].substr(tokens[10].find('(') + 1, tokens[10].find(',') - tokens[10].find('(') - 1));
							float mb = std::stof(tokens[11].substr(0, tokens[11].find(',')));
							float mc = std::stof(tokens[12].substr(0, tokens[12].find(',')));

							list.add(new Object(
								new Sphere(Vec3(sx, sy, sz), sr),
								new Diffuse(Vec3(ma, mb, mc))
							));

							std::cout << "Diffuse " << sx << " " << sy << " " << sz << " " << sr << " " << ma << " " << mb << " " << mc << "\n";
						} else {
							std::cerr << "Error: Material desconocido o formato incorrecto en la linea: " << line << std::endl;
						}
					} catch (const std::invalid_argument& e) {
						std::cerr << "Error: Conversion invalida en la linea: " << line << " - " << e.what() << std::endl;
					} catch (const std::out_of_range& e) {
						std::cerr << "Error: Valor fuera de rango en la linea: " << line << " - " << e.what() << std::endl;
					}
				} else {
					std::cerr << "Error: Formato de esfera incorrecto en la linea: " << line << std::endl;
				}
			} else {
				std::cerr << "Error: Formato de objeto incorrecto en la linea: " << line << std::endl;
			}
		}

		file.close();
	} else {
		std::cerr << "Error: No se pudo abrir el archivo: " << filename << std::endl;
	}

	return list;
}


Scene randomScene() {
	int n = 500;

	Scene list;

	list.add(new Object(
		new Sphere(Vec3(0, -1000, 0), 1000),
		new Diffuse(Vec3(0.5, 0.5, 0.5))
	));

	for (int a = -11; a < 11; a++) {
		for (int b = -11; b < 11; b++) {
			float choose_mat = Mirandom();

			Vec3 center(a + 0.9f * Mirandom(), 0.2f, b + 0.9f * Mirandom());

			if ((center - Vec3(4, 0.2f, 0)).length() > 0.9f) {
				if (choose_mat < 0.8f) {  // diffuse
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Diffuse(Vec3(Mirandom() * Mirandom(),
							Mirandom() * Mirandom(),
							Mirandom() * Mirandom()))
					));
				} else if (choose_mat < 0.95f) { // metal
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Metallic(Vec3(0.5f * (1 + Mirandom()),
							0.5f * (1 + Mirandom()),
							0.5f * (1 + Mirandom())),
							0.5f * Mirandom())
					));
				} else {  // glass
					list.add(new Object(
						new Sphere(center, 0.2f),
						new Crystalline(1.5f)
					));
				}
			}
		}
	}

	list.add(new Object(
		new Sphere(Vec3(0, 1, 0), 1.0),
		new Crystalline(1.5f)
	));

	list.add(new Object(
		new Sphere(Vec3(-4, 1, 0), 1.0f),
		new Diffuse(Vec3(0.4f, 0.2f, 0.1f))
	));

	list.add(new Object(
		new Sphere(Vec3(4, 1, 0), 1.0f),
		new Metallic(Vec3(0.7f, 0.6f, 0.5f), 0.0f)
	));

	return list;
}

// void rayTracingCPU(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
//     if (pw == -1) {
// 		pw = w;
// 	}

//     if (ph == -1) {
// 		ph = h;
// 	}

//     int patch_w = pw - px;

//     Scene world = loadObjectsFromFile("Scene1.txt");
//     world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
//     world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

//     Vec3 lookfrom(13, 2, 3);
//     Vec3 lookat(0, 0, 0);

//     float dist_to_focus = 10.0;
//     float aperture = 0.1f;

//     Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

//     std::cout << "Número de hilos (OpenMP): " << omp_get_max_threads() << std::endl;
//     double start_time = omp_get_wtime();

//     // =======================
//     // PARALELIZACIÓN POR FILAS
//     // =======================
//     #if PARALLEL_BY_ROWS
//         #pragma omp parallel for schedule(static)
//         for (int j = 0; j < (ph - py); j++) {
//             for (int i = 0; i < (pw - px); i++) {
//                 Vec3 col(0, 0, 0);

//                 for (int s = 0; s < ns; s++) {
//                     float u = float(i + px + Mirandom()) / float(w);
//                     float v = float(j + py + Mirandom()) / float(h);

//                     Ray r = cam.get_ray(u, v);
//                     col += world.getSceneColor(r);
//                 }

//                 col /= float(ns);
//                 col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

//                 img[(j * patch_w + i) * 3 + 2] = char(255.99f * col[0]);
//                 img[(j * patch_w + i) * 3 + 1] = char(255.99f * col[1]);
//                 img[(j * patch_w + i) * 3 + 0] = char(255.99f * col[2]);
//             }
//         }

//     // =========================
//     // PARALELIZACIÓN POR COLUMNAS
//     // =========================
//     #elif PARALLEL_BY_COLUMNS
//         for (int j = 0; j < (ph - py); j++) {
//             #pragma omp parallel for schedule(static)
//             for (int i = 0; i < (pw - px); i++) {
//                 Vec3 col(0, 0, 0);

//                 for (int s = 0; s < ns; s++) {
//                     float u = float(i + px + Mirandom()) / float(w);
//                     float v = float(j + py + Mirandom()) / float(h);

//                     Ray r = cam.get_ray(u, v);
//                     col += world.getSceneColor(r);
//                 }

//                 col /= float(ns);
//                 col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

//                 img[(j * patch_w + i) * 3 + 2] = char(255.99f * col[0]);
//                 img[(j * patch_w + i) * 3 + 1] = char(255.99f * col[1]);
//                 img[(j * patch_w + i) * 3 + 0] = char(255.99f * col[2]);
//             }
//         }

//     // ================================
//     // PARALELIZACIÓN POR BLOQUES RECTANGULARES
//     // ================================
//     #elif PARALLEL_BY_BLOCKS
//         int block_width  = (pw - px) / block_count_x;
//         int block_height = (ph - py) / block_count_y;

//         #pragma omp parallel for collapse(2) schedule(static)
//         for (int block_y = 0; block_y < block_count_y; block_y++) {
//             for (int block_x = 0; block_x < block_count_x; block_x++) {
//                 int start_j = block_y * block_height;
//                 int end_j   = (block_y == block_count_y - 1) ? (ph - py) : (start_j + block_height);

//                 int start_i = block_x * block_width;
//                 int end_i   = (block_x == block_count_x - 1) ? (pw - px) : (start_i + block_width);

//                 for (int j = start_j; j < end_j; j++) {
//                     for (int i = start_i; i < end_i; i++) {
//                         Vec3 col(0, 0, 0);

//                         for (int s = 0; s < ns; s++) {
//                             float u = float(i + px + Mirandom()) / float(w);
//                             float v = float(j + py + Mirandom()) / float(h);

//                             Ray r = cam.get_ray(u, v);
//                             col += world.getSceneColor(r);
//                         }

//                         col /= float(ns);
//                         col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));
//                         img[(j * patch_w + i) * 3 + 2] = char(255.99f * col[0]);
//                         img[(j * patch_w + i) * 3 + 1] = char(255.99f * col[1]);
//                         img[(j * patch_w + i) * 3 + 0] = char(255.99f * col[2]);
//                     }
//                 }
//             }
//         }
//     #endif

//     double end_time = omp_get_wtime();
//     std::cout << "Tiempo de renderizado (OpenMP): " << (end_time - start_time) << " segundos." << std::endl;
// }

void renderPixel(unsigned char* img, int patch_w, int i, int j, int w, int h, int px, int py, int ns, Camera& cam, Scene& world) {
    Vec3 col(0, 0, 0);

    for (int s = 0; s < ns; s++) {
        float u = float(i + px + Mirandom()) / float(w);
        float v = float(j + py + Mirandom()) / float(h);
		
        Ray r = cam.get_ray(u, v);
        col += world.getSceneColor(r);
    }

    col /= float(ns);
    col = Vec3(sqrt(col[0]), sqrt(col[1]), sqrt(col[2]));

    img[(j * patch_w + i) * 3 + 2] = char(255.99f * col[0]);
    img[(j * patch_w + i) * 3 + 1] = char(255.99f * col[1]);
    img[(j * patch_w + i) * 3 + 0] = char(255.99f * col[2]);
}

void rayTracingCPU(unsigned char* img, int w, int h, int ns = 10, int px = 0, int py = 0, int pw = -1, int ph = -1) {
    if (pw == -1) pw = w;
    if (ph == -1) ph = h;

    int patch_w = pw - px;

    Scene world = loadObjectsFromFile("Scene1.txt");
    world.setSkyColor(Vec3(0.5f, 0.7f, 1.0f));
    world.setInfColor(Vec3(1.0f, 1.0f, 1.0f));

    Vec3 lookfrom(13, 2, 3);
    Vec3 lookat(0, 0, 0);
    float dist_to_focus = 10.0;
    float aperture = 0.1f;

    Camera cam(lookfrom, lookat, Vec3(0, 1, 0), 20, float(w) / float(h), aperture, dist_to_focus);

    std::cout << "Número de hilos (OpenMP): " << omp_get_max_threads() << std::endl;
    double start_time = omp_get_wtime();

    // ==========================
    // PARALELIZACIÓN POR FILAS (j)
    // ==========================
    #if PARALLEL_BY_ROWS
        std::cout << "Paralelización por FILAS (j)\n";
        #pragma omp parallel for schedule(static)
        for (int j = 0; j < (ph - py); j++) {
            for (int i = 0; i < (pw - px); i++) {
                renderPixel(img, patch_w, i, j, w, h, px, py, ns, cam, world);
            }
        }

    // ============================
    // PARALELIZACIÓN POR COLUMNAS (i)
    // ============================
    #elif PARALLEL_BY_COLUMNS
        std::cout << "Paralelización por COLUMNAS (i)\n";
        for (int j = 0; j < (ph - py); j++) {
            #pragma omp parallel for schedule(static)
            for (int i = 0; i < (pw - px); i++) {
                renderPixel(img, patch_w, i, j, w, h, px, py, ns, cam, world);
            }
        }

    // =========================================
    // PARALELIZACIÓN POR BLOQUES RECTANGULARES
    // =========================================
    #elif PARALLEL_BY_BLOCKS
        std::cout << "Paralelización por BLOQUES RECTANGULARES\n";
        int block_width  = (pw - px) / block_count_x;
        int block_height = (ph - py) / block_count_y;

        int total_blocks = block_count_x * block_count_y;
		
        #pragma omp parallel for schedule(static)
        for (int block_idx = 0; block_idx < total_blocks; block_idx++) {
            int block_y = block_idx / block_count_x;
            int block_x = block_idx % block_count_x;

            int start_j = block_y * block_height;
            int end_j   = (block_y == block_count_y - 1) ? (ph - py) : (start_j + block_height);

            int start_i = block_x * block_width;
            int end_i   = (block_x == block_count_x - 1) ? (pw - px) : (start_i + block_width);

            for (int j = start_j; j < end_j; j++) {
                for (int i = start_i; i < end_i; i++) {
                    renderPixel(img, patch_w, i, j, w, h, px, py, ns, cam, world);
                }
            }
        }
    #endif

    double end_time = omp_get_wtime();
    std::cout << "Tiempo de renderizado (OpenMP): " << (end_time - start_time) << " segundos." << std::endl;
}

int main(int argc, char* argv[]) {
	/* 
	// MAIN PARA OPENMP
	//srand(time(0));

	int w = 256;// 1200;
	int h = 256;// 800;
	int ns = 10;

	int patch_x_size = w;
	int patch_y_size = h;
	int patch_x_idx = 1;
	int patch_y_idx = 1;

	int size = sizeof(unsigned char) * patch_x_size * patch_y_size * 3;
	unsigned char* data = (unsigned char*)calloc(size, 1);

	int patch_x_start = (patch_x_idx - 1) * patch_x_size;
	int patch_x_end = patch_x_idx * patch_x_size;
	int patch_y_start = (patch_y_idx - 1) * patch_y_size;
	int patch_y_end = patch_y_idx * patch_y_size;

	printParallelStrategy();

	rayTracingCPU(data, w, h, ns, patch_x_start, patch_y_start, patch_x_end, patch_y_end);

	writeBMP("imgCPUImg1.bmp", data, patch_x_size, patch_y_size);
	printf("Imagen creada.\n");

	free(data);
	//getchar();
	return (0);
	*/

	// MAIN PARA MPI
	// Inicializar MPI
    MPI_Init(&argc, &argv);

    int rank, size;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int w = 256;
    int h = 256;
    int ns = 10;

    // ======= Cálculo del reparto dinámico de filas =======
    int rows_per_process = h / size;
    int remainder = h % size;

    int start_row = rank * rows_per_process + std::min(rank, remainder);
    int end_row = start_row + rows_per_process + (rank < remainder ? 1 : 0);
    int local_rows = end_row - start_row;

    // Tamaño del buffer local
    int local_size = w * local_rows * 3;
    unsigned char* local_data = (unsigned char*)calloc(local_size, 1);

    // Reparto (solo el máster necesita estos arrays)
    int* recvcounts = nullptr;
    int* displs = nullptr;

    if (rank == 0) {
        recvcounts = new int[size];
        displs = new int[size];

        int offset = 0;

        for (int i = 0; i < size; i++) {
            int rows = h / size + (i < remainder ? 1 : 0);

            recvcounts[i] = rows * w * 3;
            displs[i] = offset;
            offset += recvcounts[i];
        }
    }

    // ========== Render (cada proceso solo hace sus filas) ==========
    printParallelStrategy();
    rayTracingCPU(local_data, w, h, ns, 0, start_row, w, end_row);

    // ========== Reunir los trozos con Gatherv ==========
    unsigned char* global_data = nullptr;

    if (rank == 0) {
        global_data = (unsigned char*)calloc(w * h * 3, 1);
    }

    MPI_Gatherv(
        local_data,
        local_rows * w * 3,
        MPI_UNSIGNED_CHAR,
        global_data,
        recvcounts,
        displs,
        MPI_UNSIGNED_CHAR,
        0,
        MPI_COMM_WORLD
    );

    // ========== El máster escribe la imagen ==========
    if (rank == 0) {
        writeBMP("imgMPI_hibrido.bmp", global_data, w, h);
        printf("Imagen creada (MPI + OpenMP).\n");

        delete[] recvcounts;
        delete[] displs;

        free(global_data);
    }

    free(local_data);

    MPI_Finalize();

    return 0;
}