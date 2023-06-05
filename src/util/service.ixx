export module minote.service;

import <stdexcept>;
import <utility>;

// Service wrapper implementation
// A service instance needs to be provided by the user, instantiating
// the underlying class
// provide() returns a stub, the service becomes unavailable again
// once the stub goes out of scope
// To use the service, a class must inherit the service
export template<typename T>
class Service {
    using Self = Service<T>;
    using Wrapped = T;

    inline static Wrapped* current_serv = nullptr;

protected:
    Wrapped*& serv = current_serv;

    Service() { if (!serv) throw std::runtime_error("Service not available when requested"); }

public:
    class Provider {
    public:
        template<typename... Args>
        Provider(Args&&... args):
            inst(std::forward<Args>(args)...),
            prev(current_serv)
        {
            current_serv = &inst;
        }

        ~Provider() { if (prev) current_serv = prev; }

        // Moveable
        Provider(Provider&& other) :
            inst(std::move(other.inst)),
            prev(other.prev)
        {
            other.prev = nullptr;
            current_serv = &inst;
        }
        auto operator=(Provider&& other) -> Provider& {
            inst = std::move(other.inst);
            prev = other.prev;
            other.prev = nullptr;
            current_serv = &inst;
        }

    private:
        Wrapped inst;
        Wrapped* prev;
    };
};
