#include <typeinfo>
#include <memory>

class TypeChecker {
public:
    template <typename Base, typename Derived>
    static bool IsInstanceOf(const Derived& derivedObj) {
        return dynamic_cast<const Base*>(&derivedObj) != nullptr;
    }

    template <typename Base, typename Derived>
    static bool IsInstanceOf(const Derived* derivedPtr) {
        return dynamic_cast<const Base*>(derivedPtr) != nullptr;
    }

    template <typename Base, typename Derived>
    static bool IsInstanceOf(const std::shared_ptr<Derived>& derivedPtr) {
        return std::dynamic_pointer_cast<Base>(derivedPtr) != nullptr;
    }
};
