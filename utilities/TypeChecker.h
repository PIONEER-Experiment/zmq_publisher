#include <typeinfo>

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
};
