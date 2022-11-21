#include <iostream>
#include <thread>
#include <atomic>
#include <map>
#include <stdatomic.h>
#include <cassert>
#include <sstream>
#include <utility>

class KCSS {
private:

	struct loc_t_b {
		loc_t_b() :
				tid(0), val(0) {
		}
		loc_t_b(uint64_t v) :
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

	bool tagged_id(uint64_t v) {
		return (v & 0x0000000000000001) == 1;
	}

	inline bool CAS(uint64_t *p, uint64_t exp, uint64_t des) {
		return __sync_bool_compare_and_swap(p, exp, des);
	}

	uint16_t id(uint64_t v) {
		TagType vt;
		*reinterpret_cast<uint64_t*>(&vt) = static_cast<uint64_t>(v);
		return vt.pid;
	}

	void reset(loc_t_b *a) {
		uint64_t oldval = a->val;
		if (tagged_id(oldval)) {
			CAS(&a->val, oldval, VAL_SAVED[id(oldval)]);  // CAS
		}
	}

	uint64_t read(loc_t_b *a) {
		while (true) {
			uint64_t val = a->val;
			if (!tagged_id(val))
				return val;
			reset(a);
		}
	}

	uint64_t create_tagged_value(uint16_t pid, uint64_t tag) {
		TagType vt;
		vt.pid = pid;
		vt.tag = tag;
		vt.tagged = 1;

		return *reinterpret_cast<uint64_t*>(&vt);
	}

	uint16_t thread_id() {
		thread_local static uint16_t id = _thread_id++;
		return id;
	}

	uint64_t ll(loc_t_b *a) {
		while (true) {
			TAGS[thread_id()]++;
			uint64_t oldVal = read(a);
			VAL_SAVED[thread_id()] = oldVal;
			uint64_t tag_id = create_tagged_value(thread_id(),
					TAGS[thread_id()]);
			if (CAS(&a->val, oldVal, tag_id)) {
				a->tid = tag_id;
				return oldVal;
			}
		}
	}

	bool sc(loc_t_b *a, uint64_t newval) {
		uint64_t tag_id = create_tagged_value(thread_id(), TAGS[thread_id()]);
		return CAS(&a->val, tag_id, newval);
	}

	void collect_taggeds_ids(const std::size_t k, loc_t_b **a, uint64_t *ta) {
		for (unsigned int i = 0; i < k; ++i) {
			ta[i] = a[i]->tid;
		}
	}

	void collect_values(const std::size_t k, loc_t_b **a, uint64_t *va) {
		for (unsigned int i = 0; i < k; ++i) {
			va[i] = read(a[i]);
		}
	}

	void snapshot(const std::size_t k, loc_t_b **a, uint64_t *va) {
		uint64_t ta[k], tb[k];
		uint64_t vb[k];

		while (true) {
			collect_taggeds_ids(k, a, ta);
			collect_values(k, a, va);
			collect_values(k, a, vb);
			collect_taggeds_ids(k, a, tb);

			unsigned i = 0;
			while (i < k && ta[i] == tb[i] && va[i] == vb[i])
				++i;

			if (i == k)
				return;
		}

		return;
	}

public:

	KCSS() :
			VAL_SAVED(), TAGS(), _thread_id() {
	}

	template<typename T, class Enabled = void>
	struct loc_t;

	// uint64_t and int64_t
	template<typename T>
	struct loc_t<T,
			std::enable_if_t<
					std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>>> : loc_t_b {
		struct bla {
			T p :1;
			T v :63;
		};

		loc_t() :
				loc_t_b() {
		}

		loc_t(T v) :
				loc_t_b(to_value_t(v)) {
		}

		inline uint64_t to_value_t(T v) {
			union {
				bla x;
				uint64_t _x;
			};
			x.p = 0;
			x.v = v;
			return _x;
		}

		inline T from_value_t(uint64_t v) {
			union {
				bla x;
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
							&& (sizeof(T) < 8)>> : loc_t_b {

		loc_t() :
				loc_t_b() {
		}

		loc_t(T v) :
				loc_t_b(to_value_t(v)) {
		}

		inline uint64_t to_value_t(T v) {

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

		inline T from_value_t(uint64_t v) {
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

	// trying to handle double by taking 1 bit from the mantissa, does not work well since
	// the values in the mantissa are really weird (they are very large even for small values)
	//
	// FIXME template<> 
	struct loc_t<double> : loc_t_b {

		loc_t() :
				loc_t_b() {
		}

		loc_t(double v) :
				loc_t_b(to_value_t(v)) {
		}

		struct double_layout {
			uint64_t mantisa :52;
			uint64_t exp :11;
			uint64_t sign :1;
		};

		struct double63_layout {
			uint64_t p :1;
			uint64_t mantisa :51;
			uint64_t exp :11;
			uint64_t sign :1;
		};

		inline uint64_t to_value_t(double v) {
			union {
				double_layout d1;
				double a1;
			};
			union {
				double63_layout d2;
				uint64_t a2;
			};

			a1 = v;
			d2.p = 0;
			d2.sign = d1.sign;
			d2.exp = d1.exp;
			d2.mantisa = d1.mantisa;

			return a2;

		}

		inline double from_value_t(uint64_t v) {
			union {
				double_layout d1;
				double a1;
			};
			union {
				double63_layout d2;
				uint64_t a2;
			};
			a2 = v;
			d1.sign = d2.sign;
			d1.exp = d2.exp;
			d1.mantisa = d2.mantisa;
			return a1;
		}
	};

	// pointers
	template<typename T>
	struct loc_t<T*> : loc_t_b {

		loc_t() :
				loc_t_b() {
		}

		loc_t(T v) :
				loc_t_b(to_value_t(v)) {
		}

		inline uint64_t to_value_t(T *v) {
			static_assert( (alignof(T) & 1) == 0 );
			// T's alignof must be even
			assert((reinterpret_cast<uint64_t>(v) & 1) == 0);// pointer must be aligned to even address
			return reinterpret_cast<uint64_t>(v);
		}
		inline T* from_value_t(uint64_t v) {
			return reinterpret_cast<T*>(v);
		}
	};

	template<typename T>
	static std::pair<loc_t<T>&, T> mp(loc_t<T> &a, T b) {
		return std::move(std::pair<loc_t<T>&, T>(a, b));
	}

	template<typename T>
	inline T get(loc_t<T> &a) {
		return a.from_value_t(read(&a));
	}

	template<typename T0, typename ...Ts>
	bool kcss(loc_t<T0> &a0, T0 a0_expv, T0 a0_newv, Ts &&...args) {

		constexpr std::size_t k = sizeof...(args) + 1;

		uint64_t oldValues[k];
		uint64_t expVals[k] = { a0.to_value_t(a0_expv),
				(args.first.to_value_t(args.second))... };
		loc_t_b *a[k] = { &a0, (&args.first)... };
		uint64_t newVal = a0.to_value_t(a0_newv);

		while (true) {
			oldValues[0] = ll(a[0]);
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

