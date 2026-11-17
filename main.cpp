#include <cmath>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

double checkedArea(double a, double b, double c) {
    if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c))
        throw std::invalid_argument("Sides must be finite");
    if (a <= 0 || b <= 0 || c <= 0)
        throw std::invalid_argument("Sides must be positive");
    const long double x = a, y = b, z = c;
    if (x + y <= z || x + z <= y || y + z <= x)
        throw std::invalid_argument("Triangle inequality is not satisfied");
    const long double p = (x + y + z) / 2;
    const double area = static_cast<double>(std::sqrt(p * (p-x) * (p-y) * (p-z)));
    if (!std::isfinite(area) || area <= 0)
        throw std::invalid_argument("Area is outside the representable range");
    return area;
}

double areaPlain(double a, double b, double c) { return checkedArea(a, b, c); }

// throw() forbids an exception from escaping this function.
double areaNoThrow(double a, double b, double c) throw() {
    try { return checkedArea(a, b, c); }
    catch (...) { return -1.0; } // A negative area is an unambiguous error marker.
}

// Dynamic exception specifications require C++14 or an earlier standard.
double areaStandard(double a, double b, double c) throw(std::invalid_argument) {
    return checkedArea(a, b, c);
}

class EmptyError {};
class ParameterError {
public:
    double a, b, c;
    std::string reason;
    ParameterError(double x, double y, double z, const char* text)
        : a(x), b(y), c(z), reason(text) {}
};
class TriangleError : public std::exception {
    std::string reason_;
public:
    double a, b, c;
    TriangleError(double x, double y, double z, const char* text)
        : reason_(text), a(x), b(y), c(z) {}
    const char* what() const noexcept override { return reason_.c_str(); }
};

double areaEmpty(double a, double b, double c) throw(EmptyError) {
    try { return checkedArea(a, b, c); }
    catch (const std::invalid_argument&) { throw EmptyError(); }
}
double areaParameters(double a, double b, double c) throw(ParameterError) {
    try { return checkedArea(a, b, c); }
    catch (const std::invalid_argument& e) { throw ParameterError(a,b,c,e.what()); }
}
double areaDerived(double a, double b, double c) throw(TriangleError) {
    try { return checkedArea(a, b, c); }
    catch (const std::invalid_argument& e) { throw TriangleError(a,b,c,e.what()); }
}

int main() {
    std::cout << std::fixed << std::setprecision(2);
    bool ok = true;
    const double valid[] = {
        areaPlain(3,4,5), areaNoThrow(3,4,5), areaStandard(3,4,5),
        areaEmpty(3,4,5), areaParameters(3,4,5), areaDerived(3,4,5)
    };
    const char* names[] = {"plain", "throw()", "standard", "empty", "parameters", "derived"};
    for (int i = 0; i < 6; ++i) {
        std::cout << names[i] << ": area(3,4,5) = " << valid[i] << '\n';
        ok = ok && std::fabs(valid[i] - 6.0) < 1e-10;
    }
    const double bad[][3] = {{0,3,4}, {-1,3,4}, {1,2,3},
                            {std::numeric_limits<double>::infinity(),3,4}};
    for (const auto& v : bad) {
        bool caught = false;
        try { areaPlain(v[0],v[1],v[2]); }
        catch (const std::invalid_argument& e) {
            caught = true;
            std::cout << "plain caught: " << e.what() << '\n';
        }
        ok = ok && caught;
    }
    bool standard = false, empty = false, parameters = false, derived = false;
    try { areaStandard(1,2,3); }
    catch (const std::invalid_argument& e) {
        standard = true; std::cout << "standard caught: " << e.what() << '\n';
    }
    const double marker = areaNoThrow(1,2,3);
    std::cout << "throw(): error handled inside; marker = " << marker << '\n';
    try { areaEmpty(1,2,3); }
    catch (const EmptyError&) {
        empty = true; std::cout << "main caught EmptyError\n";
    }
    try { areaParameters(1,2,3); }
    catch (const ParameterError& e) {
        parameters = e.a == 1 && e.b == 2 && e.c == 3;
        std::cout << "main caught ParameterError(" << e.a << ',' << e.b << ',' << e.c
                  << "): " << e.reason << '\n';
    }
    try { areaDerived(1,2,3); }
    catch (const TriangleError& e) {
        derived = e.a == 1 && e.b == 2 && e.c == 3;
        const std::exception& base = e;
        std::cout << "main caught TriangleError; std::exception::what(): " << base.what() << '\n';
    }
    ok = ok && standard && empty && parameters && derived && marker == -1.0;
    std::cout << "Self-checks: " << (ok ? "PASS" : "FAIL") << '\n';
    return ok ? 0 : 1;
}
