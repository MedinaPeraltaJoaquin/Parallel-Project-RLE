#ifndef TIMER_HPP
#define TIMER_HPP

#include <chrono>

/**
 * @brief Clase simple para medir el tiempo de ejecución en segundos.
 */
class Timer {
public:
    Timer() : start_time_point(std::chrono::high_resolution_clock::now()) {}

    /**
     * @brief Detiene el timer y retorna el tiempo transcurrido.
     * @return Tiempo transcurrido en segundos (double).
     */
    double stop() {
        auto end_time_point = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time_point - start_time_point);
        return duration.count() / 1000000.0;
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> start_time_point;
};

#endif