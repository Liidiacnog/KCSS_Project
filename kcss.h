#include <iostream>
#include <thread>
#include <atomic>
#include <map>
#include <cassert>
#include <sstream>
#include <utility>

class KCSS {
private:

	struct loc_t_base {
		loc_t_base() noexcept :
				tid(0), val(0) {
		}
		loc_t_base(uint64_t v) noexcept :
				tid(0), val(v) {
		}
		uint64_t tid;
		uint64_t val;
	};

	struct TagType {
		uint64_t tagged :1;
		uint64_t pid :15;
		uint64_t tag :48;
	};
	static_assert( sizeof(TagType) == 8 );

	constexpr inline bool tagged_id(uint64_t v) const noexcept {
		return (v & 0x0000000000000001) == 1;
	}

	inline bool cas(uint64_t *p, uint64_t exp, uint64_t des) noexcept {
		return __sync_bool_compare_and_swap(p, exp, des);
	}

	constexpr inline uint16_t id(uint64_t v) const noexcept {
		union {
			TagType vt;
			uint64_t a;
		};
		a = v;
		return vt.pid;
	}

	inline void reset(loc_t_base *a) noexcept {
		uint64_t oldval = a->val;
		if (tagged_id(oldval)) {
			cas(&a->val, oldval, VAL_SAVED[id(oldval)]);  // CAS
		}
	}

	inline uint64_t read(loc_t_base *a) noexcept {
		while (true) {
			uint64_t val = a->val;
			if (!tagged_id(val))
				return val;
			reset(a);
		}
	}

	inline constexpr uint64_t create_tagged_value(uint16_t pid,
			uint64_t tag) const noexcept {
		union {
			TagType vt;
			uint64_t a;
		};

		vt.pid = pid;
		vt.tag = tag;
		vt.tagged = 1;

		return a;
	}

	uint16_t thread_id() noexcept {
		thread_local static uint16_t id = _thread_id++;
		return id;
	}

	inline uint64_t ll(loc_t_base *a) noexcept {
		while (true) {
			TAGS[thread_id()]++;
			uint64_t oldVal = read(a);
			VAL_SAVED[thread_id()] = oldVal;
			uint64_t tag_id = create_tagged_value(thread_id(),
					TAGS[thread_id()]);
			if (cas(&a->val, oldVal, tag_id)) {
				a->tid = tag_id;
				return oldVal;
			}
		}
	}

	inline bool sc(loc_t_base *a, uint64_t newval) noexcept {
		uint64_t tag_id = create_tagged_value(thread_id(), TAGS[thread_id()]);
		return cas(&a->val, tag_id, newval);
	}

/* 	template<std::size_t k>
	inline constexpr void collect_tagged_ids_(loc_t_base **a,
			uint64_t *ta) const noexcept {
		collect_tagged_ids_<k - 1>(a, ta);
		ta[k - 1] = a[k - 1]->tid;
	}

	template<>
	inline constexpr void collect_tagged_ids_<0>(loc_t_base**,
			uint64_t*) const noexcept {
	}

	template<std::size_t k, std::enable_if_t< (k > 0) >>
	inline void collect_values_(loc_t_base **a, uint64_t *va) noexcept {
		collect_values_<k - 1>(a, va);
		va[k - 1] = read(a[k - 1]);
	}

	template<std::size_t k, std::enable_if_t< (k == 0) >>
	inline void collect_values_(loc_t_base**, uint64_t*) noexcept {
	}

	template<std::size_t k, std::enable_if_t< (k > 0) >>
	inline bool eval_cond_(uint64_t *va, uint64_t *vb, uint64_t *ta,
			uint64_t *tb) noexcept {
		return eval_cond_<k - 1>(va, vb, ta, tb) && ta[k - 1] == tb[k - 1]
				&& va[k - 1] == vb[k - 1];
	}

	template<std::size_t k, std::enable_if_t< (k == 0) >>
	inline bool eval_cond_(uint64_t*, uint64_t*, uint64_t*,
			uint64_t*) noexcept {
		return true;
	}

	template<std::size_t k, std::enable_if_t< (k > 0) >>
	inline void snapshot_(loc_t_base **a, uint64_t *va) noexcept {
		uint64_t ta[k], tb[k];
		uint64_t vb[k];

		while (true) {
			collect_tagged_ids_<k>(a, ta);
			collect_values_<k>(a, va);
			collect_values_<k>(a, vb);
			collect_tagged_ids_<k>(a, tb);

			if (eval_cond_<k>(va, vb, ta, tb))
				return;
		}

		return;
	}

	template<std::size_t k, std::enable_if_t < (k == 0) >>
	inline void snapshot_(loc_t_base**, uint64_t*) noexcept {
	}
 */
	inline constexpr void collect_tagged_ids(const std::size_t k,
			loc_t_base **a, uint64_t *ta) const noexcept {
		for (unsigned int i = 0; i < k; ++i) {
			ta[i] = a[i]->tid;
		}
	}

	inline void collect_values(const std::size_t k, loc_t_base **a,
			uint64_t *va) noexcept {
		for (unsigned int i = 0; i < k; ++i) {
			va[i] = read(a[i]);
		}
	}

	inline void snapshot(std::size_t k, loc_t_base **a, uint64_t *va) noexcept {
		uint64_t ta[k], tb[k];
		uint64_t vb[k];

		while (true) {
			collect_tagged_ids(k, a, ta);
			collect_values(k, a, va);
			collect_values(k, a, vb);
			collect_tagged_ids(k, a, tb);

			unsigned i = 0;
			while (i < k && ta[i] == tb[i] && va[i] == vb[i])
				++i;

			if (i == k)
				return;
		}

		return;
	}

	template<std::size_t k, std::enable_if_t < (k > 0) >>
	inline bool eval_kcss_cond_(uint64_t *oldValues, uint64_t *expVals) noexcept {
		return eval_kcss_cond_<k - 1>(oldValues, expVals)
				|| oldValues[k - 1] != expVals[k - 1];
	}

	template<std::size_t k, std::enable_if_t < (k == 0) >>
	inline bool eval_kcss_cond_(uint64_t*, uint64_t*) noexcept {
		return false;
	}

public:

	KCSS() noexcept :
			VAL_SAVED(), TAGS(), _thread_id() {
	}

	template<typename T, class Enabled = void>
	struct loc_t;


// uint64_t and int64_t
	template<typename T>
	struct loc_t<T,
			std::enable_if_t<
					std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>>> : loc_t_base {
		struct t63 {
			T p :1;
			T v :63;
		};

		loc_t() noexcept :
				loc_t_base() {
		}

		loc_t(T v) noexcept :
				loc_t_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T v) const noexcept {
			union {
				t63 x;
				uint64_t _x;
			};
			x.p = 0;
			x.v = v;
			return _x;
		}

		constexpr inline T from_value_t(uint64_t v) const noexcept {
			union {
				t63 x;
				uint64_t _x;
			};
			_x = v;
			return x.v;
		}

	};

// any numeric type of size smaller than 8 bytes
	template<typename T>
	struct loc_t<T,
			std::enable_if_t<
					(std::is_integral_v<T> || std::is_floating_point_v<T>)
							&& (sizeof(T) < 8)>> : loc_t_base {

		loc_t() noexcept :
				loc_t_base() {
		}

		loc_t(T v) noexcept :
				loc_t_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T v) const noexcept {

			union {
				struct {
					uint8_t p;
					T a2;
				} t;
				uint64_t a1;
			};
			a1 = 0;
			t.a2 = v;

			return a1;
		}

		constexpr inline T from_value_t(uint64_t v) const noexcept {
			union {
				struct {
					uint8_t p;
					T a2;
				} t;
				uint64_t a1;
			};
			a1 = v;
			return t.a2;
		}

	};

// Trying to handle double by stealing 1 bit from the mantissa. We steal the least
// significant bit which is the one affects the precision less, since it contributes
// 2^(-52) to the decimals -- see the following for more information:
//
//   https://en.wikipedia.org/wiki/Double-precision_floating-point_format
//
	template<typename T>
	struct loc_t<T,  //TODO does this work?
					std::enable_if_t<std::is_same_v<T, double>>> : loc_t_base {

		loc_t() noexcept :
				loc_t_base() {
		}

		loc_t(double v) noexcept :
				loc_t_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(double v) const noexcept {
			union {
				double a1;
				uint64_t a2;
			};
			a1 = v;
			a2 = a2 & 0xfffffffffffffffe; // turn the least significant bit of the mantissa to 0

			return a2;

		}

		constexpr inline double from_value_t(uint64_t v) const noexcept {
			union {
				uint64_t a2;
				double a1;
			};
			a2 = v;
			return a1;
		}
	};

// pointers
	template<typename T>
	struct loc_t<T*> : loc_t_base {

		loc_t() noexcept :
				loc_t_base() {
		}

		explicit loc_t(T *v) noexcept :
				loc_t_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T *v) const noexcept {
			static_assert( (alignof(T) & 1) == 0 );
			union {
				T *v_aux;
				uint64_t a;
			};
			v_aux = v;
			assert((a & 1) == 0); // pointer must be aligned to even address
			return a;
		}
		constexpr inline T* from_value_t(uint64_t v) const noexcept {
			union {
				T *a;
				uint64_t v_aux;
			};
			v_aux = v;
			return a;
		}
	};

	template<typename T>
	constexpr static std::pair<loc_t<T>&, T> mp(loc_t<T> &a, T b) noexcept {
		return std::move(std::pair<loc_t<T>&, T>(a, b));
	}

	template<typename T>
	inline T get(loc_t<T> &a) noexcept {
		return a.from_value_t(read(&a));
	}

	template<typename T0, typename ...Ts>
	bool kcss(loc_t<T0> &a0, T0 a0_expv, T0 a0_newv, Ts &&...args) noexcept {

		constexpr std::size_t k = sizeof...(args) + 1;

		uint64_t oldValues[k];
		uint64_t expVals[k] = { a0.to_value_t(a0_expv),
				(args.first.to_value_t(args.second))... };
		loc_t_base *a[k] = { &a0, (&args.first)... };
		uint64_t newVal = a0.to_value_t(a0_newv);

		while (true) {
			oldValues[0] = ll(a[0]);

			/* snapshot_<k - 1>(a + 1, oldValues + 1);
			if (eval_kcss_cond_<k>(oldValues, expVals)) {
				sc(a[0], oldValues[0]);
				return false;
			} */

			snapshot(k - 1, a + 1, oldValues + 1);
			for (unsigned int i = 0; i < k; ++i) {
				if (oldValues[i] != expVals[i]) {
 				sc(a[0], oldValues[0]);
					return false;
				}
			}

			if (sc(a[0], newVal)) {
				return true;
			}
		}

		return false;
	}

private:
	enum {
		MAX_THREAD_ID = 100
	};
	uint64_t VAL_SAVED[MAX_THREAD_ID];
	uint16_t TAGS[MAX_THREAD_ID];

	std::atomic<uint16_t> _thread_id;
};

