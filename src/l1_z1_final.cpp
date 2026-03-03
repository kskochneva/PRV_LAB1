#include <iostream>//ввод-вывод
#include <vector>// для матрицы
#include <chrono>//для времени
#include <boost/thread.hpp>//многопоточность
#include <atomic>//для непрерывности операци
#include <mutex>//только один поток остальные ждутт
#include <thread>//для потоков
#include <iomanip>//для setprecision

//объявляем что матрица это вектор х веткор
//чтобы каждый раз не прописывать
using Matrix = std::vector<std::vector<int>>;

Matrix generator(int N) {
    //создаем матрицу:
    // N раз повторяем вектор из N элементов
    Matrix mat(N, std::vector<int>(N));

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            mat[i][j] = rand() % 10 + 1; // чтобы небольшие числа были от 1 до 10
        }
    }
    return mat; //получили матрицу
}

//в функцию передаем копию матрицы и просим не менять
//вспомогательная функция, которая вычисляет только одну или несколько строк результирующей матрицы. Она создана специально для использования в потоках.
void compute_row(const Matrix& A, const Matrix& B,  Matrix& C, int start_row, int end_row, int N) {
	for (int row = start_row; row < end_row; ++row){
		for (int j = 0; j < N; j++) {
				int sum = 0;

				//дальше строка не менется а B вычисляем по столбцам
				for (int k = 0; k < N; k++) {
					sum += A[row][k] * B[k][j];
				}
				C[row][j] = sum ;
			}
		}
	}

//сначала напишем однопоточное умножение
//тип double так как мы в обоих функциях замеряем время
double multiply_single(const Matrix& A, const Matrix& B,  Matrix& C, int N) {
    //auto для автоматического определения типа переменной
    auto start = std::chrono::high_resolution_clock::now();
    //тот же алгоритм для row
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            C[i][j] = 0;
            for (int k = 0; k < N; ++k) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

//теперь многопоточные умножения

double multiply_with_boost(const Matrix& A, const Matrix& B,  Matrix& C, int N, int num_threads) {
    //аналогично с времнем алгоритма хроно
    //high_resolution лучшая точность
    auto start = std::chrono::high_resolution_clock::now();
    //многопоточность используеем для каждой строк
    //то есть будем считать несколько compute row одновременно
    //используем готовый алгорит м где используем compute_row
    std::vector<boost::thread> threads;


    //мы можем создавать по потоку на строку
    //но это очень затратная операция
    //поэтому будем выделять по несколько строк на поток
    int rows_per_thread = N / num_threads;  // сколько строк в среднем на поток
    int remaining_rows = N % num_threads;
    int current_row = 0;
    for (int t = 0; t < num_threads; ++t) {
    	//определяем сколько строк обработает текущий поток
    	//Если t < remaining_rows — добавляем 1 строку Иначе — добавляем 0
    	int rows_to_compute = rows_per_thread + (t < remaining_rows ? 1 : 0);
    	int start_row = current_row;
    	int end_row = start_row + rows_to_compute;
    	threads.emplace_back(compute_row, std::cref(A), std::cref(B),
    	                     std::ref(C), start_row, end_row, N);
    	//threads - это вектор std::vector<boost::thread>
    	//emplace_back() - метод создает объект прямо в памяти вектора (без копирования)
    	//push_back() копирует уже созданный объект
    	//лямбда функция
    	//захват мтариц по ссылке
    	//захват по значению нач и конечной строк и размера матрицы
    	// Поток 0: обрабатывает строки 0-249
    	// Поток 1: обрабатывает строки 250-499

        current_row += rows_to_compute; //добавляем обработанные строки к текущей позиции
    } //закрываем цикл for

    // жду завершения всех потоков
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double>(end - start).count();
}

double multiply_with_mutex(const Matrix& A, const Matrix& B, Matrix& C, int N, int num_threads) {
        auto start = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        std::mutex mtx; // мутекс для демонстрации (хотя здесь он не нужен)

        int rows_per_thread = N / num_threads;
        int remaining_rows = N % num_threads;
        int current_row = 0;

        for (int t = 0; t < num_threads; ++t) {
            int rows_to_compute = rows_per_thread + (t < remaining_rows ? 1 : 0);
            int start_row = current_row;
            int end_row = start_row + rows_to_compute;

            threads.emplace_back([&A, &B, &C, start_row, end_row, N, &mtx]() {
                for (int row = start_row; row < end_row; ++row) {
                    for (int j = 0; j < N; ++j) {
                        int sum = 0;
                        for (int k = 0; k < N; ++k) {
                            sum += A[row][k] * B[k][j];
                        }
                        // Демонстрация использования mutex
                        std::lock_guard<std::mutex> lock(mtx);
                        C[row][j] = sum;
                    }
                }
            });

            current_row += rows_to_compute;
        }

        for (auto& t : threads) {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }

//теперь функция для сравнения
bool checkResults(const Matrix& C1, const Matrix& C2, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (C1[i][j] != C2[i][j]) {
                return false;
            }
        }
    }
    return true;
}

int main()
{
    srand(time(NULL));

    std::vector<int> sizes = { 100, 500, 1000 };
    std::vector<int> thread_counts = { 2, 4, 8 };

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "========================================\n";
    std::cout << "RESULTS USING BOOST.THREAD vs STD.MUTEX\n";
    std::cout << "========================================\n\n";

    std::vector<double> single_times;

    for (int N : sizes) {
        std::cout << "Size: " << N << " x " << N << "\n";
        Matrix A = generator(N);
        Matrix B = generator(N);

        // ===== ОДНОПОТОЧНОЕ =====
        std::cout << "DOING SINGLE THREAD :\n";
        Matrix C_single(N, std::vector<int>(N));
        double time_single = multiply_single(A, B, C_single, N);
        single_times.push_back(time_single);
        std::cout << "Single thread time: " << time_single << " sec\n\n";

        std::cout << "Threads | Boost.Thread  | Std.Thread+mutex | Speedup(Boost) | Speedup(Std)\n";
        std::cout << "--------|---------------|-------------------|----------------|-------------\n";

        for (int num_threads : thread_counts) {
            // Boost.Thread
            Matrix C_boost(N, std::vector<int>(N));
            double time_boost = multiply_with_boost(A, B, C_boost, N, num_threads);
            bool correct_boost = checkResults(C_single, C_boost, N);
            double speedup_boost = time_single / time_boost;

            // std::thread with mutex
            Matrix C_mutex(N, std::vector<int>(N));
            double time_std = multiply_with_mutex(A, B, C_mutex, N, num_threads);
            bool correct_std = checkResults(C_single, C_mutex, N);
            double speedup_std = time_single / time_std;

            std::cout << std::setw(6) << num_threads << "  | "
                      << std::setw(11) << time_boost << "  | "
                      << std::setw(15) << time_std << "  | "
                      << std::setw(12) << speedup_boost << "x | "
                      << std::setw(11) << speedup_std << "x";

            if (!correct_boost || !correct_std) {
                std::cout << "  [INCORRECT!]";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
    }

    std::cout << "\n========================================================\n";
    std::cout << "CONCLUSIONS:\n";
    std::cout << "========================================================\n";
    std::cout << "1. Для маленьких матриц (N=100) накладные расходы на создание потоков\n";
    std::cout << "   превышают выигрыш от параллелизации.\n";
    std::cout << "2. Для больших матриц (N=1000) многопоточность дает значительное ускорение.\n";
    std::cout << "3. Boost.Thread и std::thread показывают схожую производительность,\n";
    std::cout << "   так как оба являются обертками над системными потоками.\n";
    std::cout << "4. Использование mutex в данной задаче избыточно, так как потоки\n";
    std::cout << "   работают с разными строками матрицы и не конфликтуют.\n";

    return 0;
}
