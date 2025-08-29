#pragma once

struct Position {
    int x;
    int y;
};

class IAnimal {
public:
    virtual ~IAnimal() = default;
    virtual std::string sound() = 0;
};
