#include <iostream>//ввод-вывод
#include <vector>// для матрицы
#include <thread>//многопоточность 
#include <chrono>//для времени

//объявляем что матрица это вектор х веткор
using Matrix = std::vector<std::vector<int>>;

Matrix generator(int N) {
    //создаем мтарицу:
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
void compute_row(const Matrix& A, const Matrix& B,  Matrix& C, int row, int N) {
    for (int j = 0; j < N; j++) {
        C[row][j] = 0;
        //дальше строка не менется а B вычисляем по столбцам 
        for (int k = 0; k < N; k++) {
            C[row][j] += A[row][k] * B[k][j];
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

double multiply_threads(const Matrix& A, const Matrix& B,  Matrix& C, int N) {
    //аналогично с времнем алгоритма хроно
    //high_resolution лучшая точность
    auto start = std::chrono::high_resolution_clock::now();
    //многопоточность используеем для каждой строк
    //то есть будем считать несколько compute row одновременно
    //используем готовый алгорит м где используем compute_row
    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        //ref и cref это изменяемая и неизменяемые ссылки, чтобы копировать
        //а не передовать напрямую
        
        threads.emplace_back(compute_row,std::cref(A),std::cref(B),std::ref(C),i,N);
    }
    // жду завершения всех потоков
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
    srand(time(NULL));//чтобы каждый раз разные случайные числа
    std::vector<int> sizes = { 100, 500, 1000 };//вектор разных значений N

    for (int N : sizes) {
        //чтобы выполнить умножение необходимо сгенериоват ьматрицы
        std::cout << "Size: " << N << " x " << N << "\n";
        Matrix A = generator(N);
        Matrix B = generator(N);

        Matrix C_threads(N, std::vector<int>(N));  // Результат многопоточного
        Matrix C_single(N, std::vector<int>(N));    // Результат однопоточного

        
       
        double timethreads = multiply_threads(A, B, C_threads, N);
        double timesingle = multiply_single(A, B, C_single, N);

        
        std::cout << "\n--- Results ---\n";
        std::cout << "Multithreads time: " << timethreads << " sec\n";
        std::cout << "Single thread time:  " << timesingle << " sec\n";

        //сравнение скорости
        double speedup = timesingle / timethreads;
        std::cout << "Difference: " << speedup << "x\n";

        //  Проверяем что все ок
        if (checkResults(C_threads, C_single, N)) {
            std::cout << " Correct\n";
        }
        else {
            std::cout << " Error!\n";
        }

        
        std::cout << "\n";
    }

    return 0;
        
    }

