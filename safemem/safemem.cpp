// safemem.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <Windows.h>
#include "safemem.h"
#include <vector>
#include <map>
#include <shared_mutex>
#include <deque>


template <class...xs> using KK = sfm::BindFront<std::vector, int>::type<xs...>;
KK<std::allocator<int>> k;

int main()
{
    using namespace std;
    //using AllocID = AllocIDType();
    // auto alloc = sfm::Allocator<int, AllocID>();
    // int* p = alloc.allocate(sizeof(int));
    // *p = 50;
    // std::cout << "Hello World!\n" << *p;
    // // alloc._ImposeReadOnly();
    // // alloc.deallocate(p, sizeof(int));
    // *p = 100;

    //using AllocID = AllocIDType();
    //auto alloc = sfm::Allocator<int, AllocID>();
    //vector<int, sfm::Allocator<int, AllocID>> vec( { 1, 2, 3, 4, 5, 6, 7 }, alloc );
    //vec.push_back(5);
    //for (auto j : vec) std::cout << j << " ";
    //// auto alloc = vec.get_allocator();
    //alloc._ImposeReadOnly();
    //// for (auto j : vec) std::cout << j << " ";
    //vec.back()++;
    //alloc._ReleaseReadOnly();

    //using AllocID = AllocIDType();
    //auto str = AllocID::GetString();
    //auto alloc = sfm::Allocator<int, AllocID>();
    //deque<int, sfm::Allocator<int, AllocID>> mp = deque<int, sfm::Allocator<int, AllocID>>({1, 4, 6}, alloc);
    //// auto alloc = vec.get_allocator();
    //alloc._ImposeReadOnly();
    //// mp.push_back(5);
    //mp.pop_back();
    //// for (auto j : vec) std::cout << j << " ";
    //alloc._ReleaseReadOnly();

    //sfm::RebindStlAllocator<
    //    std::map<int, double>, 
    //    typename sfm::BindLast<sfm::Allocator, AllocID>::type >::type;

    //using AllocID = AllocIDType();
    //auto str = AllocID::GetString();
    //auto alloc = sfm::Allocator<int, AllocID>();
    //auto* dq = _SFM_New<AllocID, deque<int>>();
    //dq->push_back(5);
    //dq->push_back(1);
    //dq->push_back(2);
    //dq->push_back(3);
    //dq->push_back(4);
    //for (auto i : *dq) std::cout << i << std::endl;
    //dq->back()++;
    //
    //// auto watchman = sfm::Watchman(dq->get_allocator());
    //{
    //    auto watchman = ErectWatchman(*dq);
    //    for (auto i : *dq) std::cout << i << std::endl;
    //}
    //dq->pop_front();
    //_SFM_Delete(dq);

    //using AllocID = AllocIDType();
    //auto str = AllocID::GetString();
    //auto alloc = sfm::Allocator<int, AllocID>();
    //// auto* dq = _SFM_New<AllocID, deque<int>>();
    //auto dq = _SFM_MakeUnique<AllocID, deque<int>>();
    //dq->push_back(5);
    //dq->push_back(1);
    //dq->push_back(2);
    //dq->push_back(3);
    //dq->push_back(4);
    //for (auto i : *dq) std::cout << i << std::endl;
    //dq->back()++;
    //
    //// auto watchman = sfm::Watchman(dq->get_allocator());
    //{
    //    auto watchman = ErectWatchman(*dq);
    //    dq->push_back(5);
    //    for (auto i : *dq) std::cout << i << std::endl;
    //}
    //dq->pop_front();
    //// _SFM_Delete(dq);

    /*using AllocID = AllocIDType();
    auto alloc = sfm::Allocator<int, AllocID>();
    auto dq = std::deque<int, sfm::Allocator<int, AllocID>>(alloc);
    dq.push_back(5);
    dq.push_back(1);
    dq.push_back(2);
    dq.push_back(3);
    dq.push_back(4);

    const int k = 50;
    int p = 41;
    int& mystery = *(int*)(addressof(p) - sizeof(int) * 2);
    mystery++;
    std::cout << mystery << " " << k << std::endl;
    std::printf("%p %p %d %d", addressof(k), addressof(p), &p - &k, sizeof(int));*/

    auto ii = SFM_MAKE_SHARED(deque<int>, initializer_list<int>{ 1, 2, 3 });
    // ii->get_allocator()._ImposeReadOnly();
    // (*ii).get_allocator()._ImposeReadOnly();
    {
        auto pkk = ErectWatchman(*ii);
    }
    // ii->pop_back();
    // auto rii = *ii;
    // auto wm = ErectWatchman(rii);
    for (auto i : *ii) std::cout << i << std::endl;
    
    //using AllocID = AllocIDType();
    //auto str = AllocID::GetString();
    //auto hash = AllocID::hash;
    //auto alloc = sfm::Allocator<int, AllocID>();
    //list<int, sfm::Allocator<int, AllocID>> mp = list<int, sfm::Allocator<int, AllocID>>({1, 4, 6}, alloc);
    //// auto alloc = vec.get_allocator();
    //alloc._ImposeReadOnly();
    //// mp.back()++;
    //// for (auto j : vec) std::cout << j << " ";
    //alloc._ReleaseReadOnly();

    /*using AllocID = AllocIDType();
    auto str = AllocID::GetString();
    auto hash = AllocID::hash;
    auto alloc = sfm::Allocator<int, AllocID>();
    int* a = alloc.allocate(sizeof(int));
    int* b = alloc.allocate(sizeof(int));
    *a = 5; *b = 10;
    assert(*a == 5 && *b == 10);
    alloc.deallocate(a, sizeof(int));
    alloc.deallocate(b, sizeof(int));*/
}
