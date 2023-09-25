#pragma once

#include <Windows.h>
#include <list>
#include <memory>
#include <algorithm>
#include <numeric>
#include <cassert>
#include <string>
#include <type_traits>
#include <utility>
#include <map>
#include <shared_mutex>
#include <tuple>
#include <deque>
#include <functional>

#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/thread/synchronized_value.hpp>

/// -*-*-*-*-*-*-*-*-* This bit of macro cleverness is credited to Evgeny Panasyuk [ https://stackoverflow.com/a/15859077 ] -*-*-*-*-*-*-*-*-*-
#define GET_STR_AUX(_, i, str) (sizeof(str) > (i) ? str[(i)] : 0), 
#define GET_STR(str) BOOST_PP_REPEAT(128, GET_STR_AUX, str) 0
/// -*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*


namespace sfm
{
	using namespace std;

#define TO_STRING_IMPL(x) #x
#define TO_STRING(x) TO_STRING_IMPL(x)
#define AllocIDType() sfm::AllocatorID< GET_STR( __FUNCSIG__ TO_STRING(__LINE__) __FILE__ __DATE__ ) >

	/// <summary>
	/// `MosesString` is a template class used to store strings at compile-time.
	/// Strings stored in `MosesString` are	`set in stone` in some sense (hence `Moses`)
	/// The motivation of `MosesString` is basically to give some sense of `identity` to a type.
	/// (e.g. the type MosesString<'h', 'i'> is different from MosesString<'h','e','l','l','o'> )
	/// We need this to easily construct a unique type at various point in the source code by calling AllocIDType()
	/// </summary>
	template <class IntSeq, class ResSeq, class E = void> struct MosesStringImpl;
	template <class Ty, Ty Ch, Ty...Chars, Ty...ResChars>
	struct MosesStringImpl<integer_sequence<Ty, Ch, Chars...>, integer_sequence<Ty, ResChars...>, enable_if_t<(Ch != 0)>>
	{
		using type = typename MosesStringImpl<integer_sequence<Ty, Chars...>, integer_sequence<Ty, ResChars..., Ch>>::type;
	};
	template <class Ty, Ty Ch, Ty...Chars, Ty...ResChars>
	struct MosesStringImpl<integer_sequence<Ty, Ch, Chars...>, integer_sequence<Ty, ResChars...>, enable_if_t<(Ch == 0)>>
	{
		using type = integer_sequence<Ty, ResChars...>;
	};

	// Computing hash of a string at compile-time.
	template < class IntSeq > struct MosesHash;
	template < class Ty, Ty Ch, Ty...Chars > struct MosesHash<integer_sequence<Ty, Ch, Chars...> >
	{
		// Magic constant taken from Microsoft's hasing impl.
		const static size_t _FNV_prime = 1099511628211ULL;
		const static size_t hash = _FNV_prime * ((MosesHash<integer_sequence<Ty, Chars...>>::hash) ^ static_cast<size_t>(Ch));
	};
	template < class Ty, Ty Ch > struct MosesHash<integer_sequence<Ty, Ch > >
	{
		const static size_t hash = Ch;
	};

	template <char...Cs> struct MosesString : public MosesStringImpl<integer_sequence<char, Cs...>, integer_sequence<char>>
	{
		template <char...Chrs>
		static const char* _GetStringImpl(integer_sequence<char, Chrs...>)
		{
			static const char str[] = { Chrs..., '\0' };
			return str;
		}
		static const char* GetString()
		{
			return _GetStringImpl(__super::type());
		}
		// Compute hash for this string as well.
		const static size_t hash = MosesHash<integer_sequence<char, Cs...>>::hash;
	};

	struct Block
	{
	public:
		void* ptr = nullptr;
		size_t size = 0;
		bool operator==(const Block& other) const { return (ptr == other.ptr) && (size == other.size); }
	};
	
	/// <summary>
	/// We basically want the allocator to be stateful (to keep track of allocation requests),
	/// yet we also want the allocator to be `omni` in the sense that multiple copies of an 
	/// allocator of the same type AllocT shares the same collected knowledge of the allocation requests
	/// made to the type.

	/// One way to do this is to give each allocator type an shared identity so that multiple instances 
	/// of an allocator of the same type can share information about allocation requests made on it.

	/// This is necessary because MSVC's STL implementation create copies of the allocator passed to it.
	/// If we stored the information mentioned above directly in the allocator, there'd be multiple unsynchronized
	/// copies of the allocator's internal states (not desirable!). 

	/// However, if we stored the information in a static member, we risk sharing the internal states among
	/// allocators who have nothing to do with one another.

	/// </summary>
	template <char...Cs> struct AllocatorID : public MosesString<Cs...>
	{
		inline static boost::synchronized_value< list< Block, std::allocator<Block> > > _allocatedBlocks = {};
	};
	
	template < class T, class AllocID >
	struct Allocator
	{
	public:
		// rituals
		using value_type = T;
		using pointer = T*;
		using const_pointer = const T*;
		using reference = T&;
		using const_reference = const T&;
		using size_type = size_t;
		using difference_type = ptrdiff_t;
		using propagate_on_container_move_assignment = true_type;
		using is_always_equal = false_type;

		template < class U >
		struct rebind
		{
			using other = Allocator< U, AllocID >;
		};

		Allocator() = default;
		template < class U > Allocator(const Allocator< U, AllocID >& other) { };

		T* allocate(size_t n, const void* hint = 0)
		{
			auto sizeInBytes = n * sizeof(T);
			LPVOID _ptr = VirtualAlloc(NULL, sizeInBytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			if (_ptr == nullptr) throw std::bad_alloc();

			auto allocatedBlocks = AllocID::_allocatedBlocks.synchronize();

			auto it = upper_bound(
				allocatedBlocks->cbegin(), allocatedBlocks->cend(), _ptr,
				[](LPVOID ptr, const Block& block) { return ptr < block.ptr; });
			allocatedBlocks->insert(it, Block{ _ptr, sizeInBytes });
			memset(_ptr, 0xdf, sizeInBytes);

			return (T*)_ptr;
		}

		void deallocate(T* p, size_t n)
		{
			auto sizeInBytes = n * sizeof(T);
			BOOL succ = VirtualFree((LPVOID)p, sizeInBytes, MEM_DECOMMIT);
			if (!succ)
			{
				DWORD err = GetLastError();
				throw std::bad_alloc();
			}

			auto allocatedBlocks = AllocID::_allocatedBlocks.synchronize();

			auto it = find_if(allocatedBlocks->cbegin(), allocatedBlocks->cend(),
				[p, sizeInBytes](const Block& bl) { return bl.ptr == p && bl.size == sizeInBytes; });
			assert(it != allocatedBlocks->cend());
			allocatedBlocks->erase(it);
		}

		bool _ProtectAll(DWORD flNewProtect)
		{
			auto allocatedBlocks = AllocID::_allocatedBlocks.synchronize();

			bool acc = accumulate(
				allocatedBlocks->cbegin(),
				allocatedBlocks->cend(),
				true,
				[flNewProtect](bool acc, const Block& b2)
				{
					DWORD oldProtect;
					return acc && VirtualProtect((LPVOID)b2.ptr, b2.size, flNewProtect, &oldProtect);
				}
			);
			assert(acc);
			return acc;
		}
		bool _ImposeReadOnly() { return _ProtectAll(PAGE_READONLY); }
		bool _ReleaseReadOnly() { return _ProtectAll(PAGE_READWRITE); }
		bool _ImposeNoAccess() { return _ProtectAll(PAGE_NOACCESS); }
	};

	template <class T> struct IsDeque : false_type {};
	template <template <class...> class Deq, class...T > struct IsDeque<Deq<T...>> : is_same<deque<T...>, Deq<T...>> {};

	// Metafunctions currying
	// This is used to inspect and reapply each template parameter to a template class.
	template < template <class...> class Tmp, class K >
	struct BindFront { template < class...Rs > using type = Tmp< K, Rs... >; };

	template < template <class...> class Tmp, class K >
	struct BindLast { template < class...Rs > using type = Tmp< Rs..., K >; };

	template < class T > struct IsStdAllocator : false_type {};
	template < template <class...> class StdAllocator, class T > struct IsStdAllocator<StdAllocator<T>> : is_same< StdAllocator<T>, std::allocator<T> > {};

	// Use metaprogramming to rebind a STL container's allocator.
	template <class...Ts> struct LiteTuple;

	template <
		template < class... > class HollowedStlCont,
		class ScoopedOutParams,
		template <class...> class Alloc,
		class E = void> struct RebindStlAllocatorImpl;
	// If argument being inspected isn't an StdAllocator, we simply partially apply that parameter.
	template <
		template <class...> class HollowedStlCont,
		template <class...> class ScoopedOutParams,
		template <class...> class Alloc,
		class FirstArg,
		class...RestArgs>
	struct RebindStlAllocatorImpl< HollowedStlCont, ScoopedOutParams<FirstArg, RestArgs...>, Alloc,
		enable_if_t<!IsStdAllocator<FirstArg>::type::value && (sizeof...(RestArgs) > 0)>>
	{
		using type = typename RebindStlAllocatorImpl<
			typename BindFront< HollowedStlCont, FirstArg >::template type,
			ScoopedOutParams<RestArgs...>,
			Alloc >::type;
	};
	// If it is an StdAllocator, we partially apply that parameter with our sfm::Allocator.
	template <
		template <class...> class HollowedStlCont,
		template <class...> class ScoopedOutParams,
		template <class...> class Alloc,
		class FirstArg,
		class...RestArgs>
	struct RebindStlAllocatorImpl< HollowedStlCont, ScoopedOutParams<FirstArg, RestArgs...>, Alloc,
		enable_if_t<IsStdAllocator<FirstArg>::type::value && (sizeof...(RestArgs) > 0)>>
	{
		using type = typename RebindStlAllocatorImpl<
			typename BindFront< HollowedStlCont, Alloc<typename FirstArg::value_type> >::template type,
			ScoopedOutParams<RestArgs...>,
			Alloc >::type;
	};

	template <
		template <class...> class HollowedStlCont,
		template <class...> class ScoopedOutParams,
		template <class...> class Alloc,
		class LastArg,
		class...RestArgs>
	struct RebindStlAllocatorImpl< HollowedStlCont, ScoopedOutParams<LastArg, RestArgs...>, Alloc,
		enable_if_t<(sizeof...(RestArgs) == 0)>>
	{
		using type = HollowedStlCont< conditional_t< 
			IsStdAllocator<LastArg>::type::value, 
			Alloc<typename LastArg::value_type>, 
			LastArg > >;
	};

	// This will go through each template parameter of a template class and replace a STD allocator param
	// with sfm's magical allocator.
	template <class StlCont, template<class...> class Alloc> struct RebindStlAllocator
	{
		template < class T > struct HollowStlCont;
		template < template <class...> class StlCont, class...Params > struct HollowStlCont<StlCont<Params...>>
		{
			template <class...Ps> using type = StlCont<Ps...>;
			using ScoopedOutParams = LiteTuple<Params...>;
		};
		using type = typename RebindStlAllocatorImpl<
			typename HollowStlCont<StlCont>::template type,
			typename HollowStlCont<StlCont>::ScoopedOutParams,
			Alloc>::type;
	};

	
	template < class T, class E = void > struct IsSfmAllocator : false_type{};
	template < class T > struct IsSfmAllocator< T, decltype((void) &T::_ReleaseReadOnly) > : true_type {};

	// The namesake of this library.
	template <class Alloc, typename = enable_if_t<IsSfmAllocator<Alloc>::value>>
	struct Watchman
	{
	private:
		bool moved = true;
	public:
		Watchman() : moved{ false } { Alloc()._ImposeReadOnly(); };
		Watchman(Alloc alloc) : moved { false } { alloc._ImposeReadOnly(); };
		Watchman(const Watchman&) = delete;
		Watchman& operator=(const Watchman&) = delete;
		Watchman(Watchman<Alloc>&& other) { moved = false; other.moved = true; }
		~Watchman() { if (!moved) { Alloc()._ReleaseReadOnly(); } }
	};
		
}
// FYC
//template <class AllocID, class T, class...Args>
//std::enable_if_t< !sfm::IsDeque<T>::value, T* > _SFM_New(Args&&...args)
//{
//	auto alloc = sfm::Allocator<T, AllocID>();
//	T* ptr = alloc.allocate(sizeof(T));
//	return ptr;
//}


template <
	class AllocID, 
	class _T, 
	class T = typename sfm::RebindStlAllocator<_T, sfm::BindLast<sfm::Allocator, AllocID>::type>::type, 
	typename = std::enable_if_t< sfm::IsDeque<_T>::value >,
	class...Args>
T* _SFM_New(Args&&...args)
{
	auto alloc = T::allocator_type::rebind<T>::other();
	T* ptr = alloc.allocate(1);
	new (ptr) T(std::forward<Args>(args)...);
	return ptr;
}

template < class T >
std::enable_if_t < sfm::IsDeque<T>::type::value > _SFM_Delete(T* p)
{
	auto alloc = T::allocator_type::rebind<T>::other();
	alloc.deallocate(p, 1);
}

template < class AllocID, class T, class...Args >
decltype(auto) _SFM_MakeUnique(Args&&...args) 
{
	using ValType = std::remove_pointer_t< decltype(_SFM_New<AllocID, T>(std::declval<Args>()...)) >;
	auto sfmDeleter = [](ValType* p) { _SFM_Delete(p); };
	return std::unique_ptr<ValType, decltype(sfmDeleter)>( 
		_SFM_New<AllocID, T>(std::forward<Args>(args)...), sfmDeleter );
}

template < class AllocID, class T, class...Args >
decltype(auto) _SFM_MakeShared(Args&&...args)
{
	using ValType = std::remove_pointer_t< decltype(_SFM_New<AllocID, T>(std::declval<Args>()...)) >;
	auto sfmDeleter = [](ValType* p) { _SFM_Delete(p); };
	return std::shared_ptr<ValType>( _SFM_New<AllocID, T>(std::forward<Args>(args)...), sfmDeleter );
}

template < class T, typename = std::enable_if_t<sfm::IsDeque<std::remove_reference_t<T>>::value> >
auto ErectWatchman(T&& t)
{
	return sfm::Watchman(t.get_allocator());
}

#define SFM_MAKE_UNIQUE(T, ...) _SFM_MakeUnique<AllocIDType(), T>(__VA_ARGS__)
#define SFM_MAKE_SHARED(T, ...) _SFM_MakeShared<AllocIDType(), T>(__VA_ARGS__)
