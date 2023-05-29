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

	// ------------- loc_struct_base type: ------------- //

	struct loc_struct_base {
		loc_struct_base() noexcept :
				snapshot_ts(0), value(0) {
		}
		loc_struct_base(uint64_t v) noexcept :
				snapshot_ts(0), value(v) {
		}
		uint64_t snapshot_ts;
		uint64_t value;
	};

	// --------- management of TemporalValues: --------- //

	struct TemporalValue {
		uint64_t location_type  :1;
		uint64_t thread_ID		:15;
		uint64_t tag_number		:48;
	};
	static_assert( sizeof(TemporalValue) == 8 );

	constexpr inline bool is_TemporalValue(uint64_t v) const noexcept {
		return (v & 0x0000000000000001) == 1;
	}

	inline constexpr uint64_t make_TemporalValue(uint16_t thread_ID,
		uint64_t tag_number) const noexcept {
		union {
			TemporalValue vt;
			uint64_t a;
		};

		vt.thread_ID = thread_ID;
		vt.tag_number = tag_number;
		vt.location_type = 1;

		return a;
	}

	// ------------- thread ID methods: ------------- //

	uint16_t my_thread_id() noexcept {
		thread_local static uint16_t my_thread_id = _thread_id++;
		return my_thread_id;
	}

	constexpr inline uint16_t thread_id(uint64_t v) const noexcept {
		union {
			TemporalValue tv;
			uint64_t a;
		};
		a = v;
		
		return tv.thread_ID;
	}

	// ------------- methods for the KCSS algorithm: ------------- //

	inline bool cas(uint64_t* loc_pointer, uint64_t exp, uint64_t new_val) noexcept {
		return __sync_bool_compare_and_swap(loc_pointer, exp, new_val);
	}

	inline void reset(loc_struct_base *a) noexcept {
		uint64_t old_val = a->value;
		if (is_TemporalValue(old_val)) {
			cas(&a->value, old_val, SAVED_VAL[thread_id(old_val)]);
		}
	}

	inline uint64_t read(loc_struct_base *a) noexcept {
		while (true) {
			uint64_t val = a->value;
			if (!is_TemporalValue(val))
				return val;
			reset(a);
		}
	}

	inline uint64_t ll(loc_struct_base *a) noexcept {
		while (true) {
			TAG_NUMBERS[my_thread_id()]++;
			uint64_t old_val = read(a);
			SAVED_VAL[my_thread_id()] = old_val;
			uint64_t temp_val = make_TemporalValue(my_thread_id(),
					TAG_NUMBERS[my_thread_id()]);
			if (cas(&a->value, old_val, temp_val)) {
				a->snapshot_ts = temp_val;
				return old_val;
			}
		}
	}

	inline bool sc(loc_struct_base *a, uint64_t new_prog_val) noexcept {
		uint64_t temp_val = make_TemporalValue(my_thread_id(), TAG_NUMBERS[my_thread_id()]);
		return cas(&a->value, temp_val, new_prog_val);
	}

	//          collect_snapshot_ts:          //

	// Original method without iteration unfolding
	inline constexpr void collect_snapshot_ts(const std::size_t k,
			loc_struct_base **a, uint64_t *t) const noexcept {
		for (unsigned int i = 0; i < k; ++i) {
			t[i] = a[i]->snapshot_ts;
		}
	}

	// Redefined method with iteration unfolding
	template<std::size_t k>
	inline constexpr void collect_snapshot_ts_(loc_struct_base **a,
			uint64_t *t) const noexcept {
		collect_snapshot_ts_<k - 1>(a, t);
		t[k - 1] = a[k - 1]->snapshot_ts;
	}

	template<>
	inline constexpr void collect_snapshot_ts_<0>(loc_struct_base**,
			uint64_t*) const noexcept {
	}

	//          collect_values:          //
	
	// Original method without iteration unfolding
	inline void collect_values(const std::size_t k, loc_struct_base **a,
			uint64_t *v) noexcept {
		for (unsigned int i = 0; i < k; ++i) {
			v[i] = read(a[i]);
		}
	}
	
	// Redefined method with iteration unfolding
	template<std::size_t k>
	inline void collect_values_(loc_struct_base **a, uint64_t *v) noexcept {
		collect_values_<k - 1>(a, v);
		v[k - 1] = read(a[k - 1]);
	}

	template<>
	inline void collect_values_<0>(loc_struct_base**, uint64_t*) noexcept {
	}

	//          snapshot:          //

	// Original method without iteration unfolding
	inline void snapshot(std::size_t k, loc_struct_base **a, uint64_t *values_1) noexcept {
		uint64_t timestamps_1[k], timestamps_2[k];
		uint64_t values_2[k];

		while (true) {
			collect_snapshot_ts(k, a, timestamps_1);
			collect_values(k, a, values_1);
			collect_values(k, a, values_2);
			collect_snapshot_ts(k, a, timestamps_2);

			unsigned i = 0;
			while (i < k && timestamps_1[i] == timestamps_2[i] && values_1[i] == values_2[i])
				++i;

			if (i == k)
				return;
		}

		return;
	}

	// Redefined method with iteration unfolding

	// eval_cond_ method for redefined snapshot_
	template<std::size_t k>
	inline bool eval_cond_(uint64_t *values_1, uint64_t *values_2, uint64_t *timestamps_1,
			uint64_t *timestamps_2) noexcept {
		return eval_cond_<k - 1>(values_1, values_2, timestamps_1, timestamps_2) 
					&& timestamps_1[k - 1] == timestamps_2[k - 1]
					&& values_1[k - 1] == values_2[k - 1];
	}

	template<>
	inline bool eval_cond_<0>(uint64_t*, uint64_t*, uint64_t*,
			uint64_t*) noexcept {
		return true;
	}

	// snapshot_ method
	template<std::size_t k>
	inline void snapshot_(loc_struct_base **a, uint64_t *values_1) noexcept {
		uint64_t timestamps_1[k], timestamps_2[k];
		uint64_t values_2[k];

		while (true) {
			collect_snapshot_ts_<k>(a, timestamps_1);
			collect_values_<k>(a, values_1);
			collect_values_<k>(a, values_2);
			collect_snapshot_ts_<k>(a, timestamps_2);

			if (eval_cond_<k>(values_1, values_2, timestamps_1, timestamps_2))
				return;
		}

		return;
	}

	template<>
	inline void snapshot_<0>(loc_struct_base**, uint64_t*) noexcept {
	}

	//      eval_kcss_cond_ method for redefined kcss:      //
	template<std::size_t k>
	inline bool eval_kcss_cond_(uint64_t *old_values, uint64_t *exp_values) noexcept {
		return eval_kcss_cond_<k - 1>(old_values, exp_values)
				|| old_values[k - 1] != exp_values[k - 1];
	}

	template<>
	inline bool eval_kcss_cond_<0>(uint64_t*, uint64_t*) noexcept {
		return false;
	}

public:

	KCSS() noexcept :
			SAVED_VAL(), TAG_NUMBERS(), _thread_id() {
	}

	// ------------- loc_struct_t templates: ------------- //

	template<typename T, class Enabled = void>
	struct loc_struct_t;

	// loc_struct_t for uint64_t and int64_t
	template<typename T>
	struct loc_struct_t<T, std::enable_if_t< std::is_same_v<T, uint64_t> || std::is_same_v<T, int64_t>>> : loc_struct_base {
		
		struct t63 {
			T location_type :1;
			T content :63;
		};

		loc_struct_t() noexcept :
			loc_struct_base() {
		}

		loc_struct_t(T v) noexcept :
			loc_struct_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T v) const noexcept {
			union {
				t63 x;
				uint64_t _x;
			};
			x.location_type = 0;
			x.content = v;

			return _x;
		}

		constexpr inline T from_value_t(uint64_t v) const noexcept {
			union {
				t63 x;
				uint64_t _x;
			};
			_x = v;

			return x.content;
		}
	};

	// loc_struct_t for any numeric type of size smaller than 8bytes
	template<typename T>
	struct loc_struct_t<T,
			std::enable_if_t<
					(std::is_integral_v<T> || std::is_floating_point_v<T>)
							&& (sizeof(T) < 8)>> : loc_struct_base {

		loc_struct_t() noexcept :
				loc_struct_base() {
		}

		loc_struct_t(T v) noexcept :
				loc_struct_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T v) const noexcept {
			union {
				struct {
					uint8_t location_type;
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
					uint8_t location_type;
					T a2;
				} t;
				uint64_t a1;
			};
			a1 = v;

			return t.a2;
		}
	};

	// loc_struct_t for double:
	// Trying to handle double by stealing 1 bit from the mantissa. We steal the least
	// significant bit which is the one affects the precision less, since it contributes
	// 2^(-53) to the decimals -- see the following for more information:
	//
	//   https://en.wikipedia.org/wiki/Double-precision_floating-point_format
	//
	template<>
	struct loc_struct_t<double> : loc_struct_base {

		loc_struct_t() noexcept :
				loc_struct_base() {
		}

		loc_struct_t(double v) noexcept :
				loc_struct_base(to_value_t(v)) {
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

	// loc_struct_t for pointers
	template<typename T>
	struct loc_struct_t<T*> : loc_struct_base {

		loc_struct_t() noexcept :
				loc_struct_base() {
		}

		explicit loc_struct_t(T *v) noexcept :
				loc_struct_base(to_value_t(v)) {
		}

		constexpr inline uint64_t to_value_t(T *v) const noexcept {
			static_assert( (alignof(T) & 1) == 0 );
			union {
				T *v_aux;
				uint64_t a;
			};
			v_aux = v;
			assert((a & 1) == 0);//pointer must be aligned to even address

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

	// ----------- auxiliary methods get() and mp(): ----------- //

	template<typename T>
	constexpr static std::pair<loc_struct_t<T>&, T> mp(loc_struct_t<T> &a, T b) noexcept {
		return std::move(std::pair<loc_struct_t<T>&, T>(a, b));
	}

	template<typename T>
	inline T get(loc_struct_t<T> &a) noexcept {
		return a.from_value_t(read(&a));
	}

	// ----------------- KCSS methods: ----------------- //

	// Original kcss method without iteration unfolding
	template<typename T0, typename ...Ts>
	bool kcss(loc_struct_t<T0>& a0, T0 a0_expv, T0 a0_newv, Ts &&...args) noexcept {

		constexpr std::size_t k = sizeof...(args) + 1;

		uint64_t old_vals[k];
		uint64_t exp_vals[k] = { a0.to_value_t(a0_expv),
				(args.first.to_value_t(args.second))... };
		loc_struct_base* a[k] = { &a0, (&args.first)... };
		uint64_t new_v = a0.to_value_t(a0_newv);

		while (true) {
			old_vals[0] = ll(a[0]);
			snapshot(k - 1, a + 1, old_vals + 1);
			for (unsigned int i = 0; i < k; ++i) {
				if (old_vals[i] != exp_vals[i]) {
					sc(a[0], old_vals[0]);
					return false;
				}
			}

			if (sc(a[0], new_v)) {
				return true;
			}
		}

		return false;
	}

	// Redefined kcss method for iteration unfolding
	template<typename T0, typename ...Ts>
	bool kcss_(loc_struct_t<T0> &a0, T0 a0_expv, T0 a0_newv, Ts &&...args) noexcept {

		constexpr std::size_t k = sizeof...(args) + 1;

		uint64_t old_vals[k];
		uint64_t exp_vals[k] = { a0.to_value_t(a0_expv),
				(args.first.to_value_t(args.second))... };
		loc_struct_base *a[k] = { &a0, (&args.first)... };
		uint64_t new_v = a0.to_value_t(a0_newv);

		while (true) {
			old_vals[0] = ll(a[0]);

			snapshot_<k - 1>(a + 1, old_vals + 1);
			if (eval_kcss_cond_<k>(old_vals, exp_vals)) {
				sc(a[0], old_vals[0]);
				return false;
			}
			//The previous code block is equivalent to the following:
			//	snapshot(k - 1, a + 1, old_vals + 1);
			//	for (unsigned int i = 0; i < k; ++i) {
			//		if (old_vals[i] != exp_vals[i]) {
			//			sc(a[0], old_vals[0]);
			//			return false;
			//		}
			//	}

			if (sc(a[0], new_v)) {
				return true;
			}
		}

		return false;
	}

private:
	enum {
		MAX_THREAD_ID = 100
	};
	uint64_t SAVED_VAL[MAX_THREAD_ID];
	uint16_t TAG_NUMBERS[MAX_THREAD_ID];

	std::atomic<uint16_t> _thread_id;
};