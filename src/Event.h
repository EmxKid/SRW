    // include/Event.h
    #ifndef EVENT_H
    #define EVENT_H

    #include <string>
    #include <memory>
    #include <functional>

    // Типы событий для приоритезации
    enum class EventType {
        ACTIVATION,    // Переход в активную фазу (вкл / начало работы)
        DEACTIVATION,  // Переход в пассивную фазу (выкл / завершение работы)
        MONITORING     // Сбор статистики (обязательно для практики!)
    };

    // Базовое событие
    struct Event {
        double time;          // Абсолютное время возникновения
        EventType type;       // Тип для приоритезации при равенстве времени
        int userId;           // Источник события (для отладки)
        
        // Обработчик события — захватывает контекст симуляции через std::function
        std::function<void()> handler;
        
        // Оператор сравнения для приоритетной очереди (мин-куча по времени)
        bool operator>(const Event& other) const {
            if (time != other.time) return time > other.time;
            return static_cast<int>(type) > static_cast<int>(other.type); // приоритет по типу
        }
    };

    // Интерфейс обработчика событий (для компонентов системы)
    class EventHandler {
    public:
        virtual ~EventHandler() = default;
        
        // Генерация события: возвращает время его наступления
        virtual Event createArrivalEvent(double currentTime, int userId) = 0;
        virtual Event createCompletionEvent(double currentTime, int userId, double workload) = 0;
    };

    #endif // EVENT_H