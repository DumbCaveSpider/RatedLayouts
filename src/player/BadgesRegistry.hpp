#pragma once
#include <unordered_map>
#include <vector>

#include <BadgesAPI.hpp>

class BadgesRegistry
{
public:
    static std::unordered_map<int, std::vector<Badge>> &pending();
};
