#include <stdio.h> 
#include <stdlib.h> 
#include <memory>
#include "rand.h"

namespace axitrace {

static unique_ptr<rand> rand_ptr = unique_ptr<rand>(new rand());


rand::rand()
    : m_max(RAND_MAX)
{
    m_seed = (int)time(0);
    srand(m_seed);
}



rand::~rand()
{
}


rand* rand::instancep()
{
    return rand_ptr.get();
}


rand& rand::instance()
{
    return *instancep();
}


int rand::seed() const
{
    return m_seed;
}


int rand::seed(int s)
{
    m_seed = s;
    srand(m_seed);
    return m_seed;
}


int rand::max() const
{
    return m_max;
}


template<> int rand::get<int>() const
{
    return (int)::rand();
}


template<> double rand::get<double>() const
{
    double n = (double)::rand();
    return n / (double)m_max;
}






} /// namespace axitrace
