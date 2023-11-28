#ifndef TYPECHECKER_H
#define TYPECHECKER_H

#include <memory>

/**
 * @brief A utility class for type checking.
 *
 * The `TypeChecker` class provides template-based methods for checking whether
 * an object or a smart pointer is an instance of a certain base type.
 */
class TypeChecker {
public:
    /**
     * @brief Checks if a pointer is an instance of a specific base type.
     *
     * @tparam Base The base type.
     * @tparam Derived The derived type.
     * @param derivedPtr Pointer to the derived type.
     * @return True if the pointer is an instance of the base type, false otherwise.
     */
    template <typename Base, typename Derived>
    static bool IsInstanceOf(const Derived* derivedPtr) {
        return dynamic_cast<const Base*>(derivedPtr) != nullptr;
    }

    /**
     * @brief Checks if a shared pointer is an instance of a specific base type.
     *
     * @tparam Base The base type.
     * @tparam Derived The derived type.
     * @param derivedPtr Shared pointer to the derived type.
     * @return True if the shared pointer is an instance of the base type, false otherwise.
     */
    template <typename Base, typename Derived>
    static bool IsInstanceOf(const std::shared_ptr<Derived>& derivedPtr) {
        return std::dynamic_pointer_cast<Base>(derivedPtr) != nullptr;
    }
};

#endif // TYPECHECKER_H
