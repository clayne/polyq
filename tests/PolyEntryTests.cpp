#include <polyq/entry.h>

#include "helpers.h"

#include <ut/assert.h>
#include <ut/test.h>

using namespace std;

namespace polyq
{
	namespace tests
	{
		namespace
		{
			struct Foo
			{
				virtual ~Foo() { }
				virtual string get_type() const = 0;

			private:
				const Foo &operator =(const Foo &rhs);
			};

			class Bar : public Foo
			{
			public:
				Bar(const string &value_, int &count)
					: value(value_), _count(count)
				{	++_count;	}

				Bar(const Bar &other)
					: value(other.value), _count(other._count)
				{	++_count;	}

				~Bar()
				{	--_count;	}

			public:
				string value;

			private:
				virtual string get_type() const
				{	return "Bar";	}

			private:
				int &_count;
				char _padding[24];
			};

			class Baz : public Foo
			{
			public:
				Baz(int value_, int &count)
					: value(value_), _count(count)
				{	++_count;	}

				Baz(const Baz &other)
					: value(other.value), _count(other._count)
				{	++_count;	}

				~Baz()
				{	--_count;	}

			public:
				int value;

			private:
				virtual string get_type() const
				{	return "Baz";	}

			private:
				int &_count;
			};

			struct POD
			{
				char padding[101];
			};

			class VPolyA
			{
				virtual string get_type2() const = 0;
				int value;
			};

			class VPolyB
			{
			public:
				virtual ~VPolyB() {	}
				virtual string get_type2() const = 0;
				string values[3];
			};

			class VDerived : public POD, public VPolyA, public VPolyB
			{
			public:
				~VDerived()
				{	}

				virtual string get_type2() const { return "derived"; }
			};


			class BazNC : public Foo, instance_counter
			{
			public:
				BazNC(int value_, int &count)
					: value(value_), instance_counter(count)
				{	}

				BazNC(BazNC &&other)
					: value(other.value), instance_counter(static_cast<instance_counter &&>(other))
				{	other.value = -1;	}

			public:
				int value;

			private:
				virtual string get_type() const
				{	return "BazNC";	}
			};
		}

		begin_test_suite( PolyEntryTests )
			vector<byte> buffer;
			byte *begin, *end;
			byte *p;

			init( InitBufferAndBoundaries )
			{
				VDerived d;
				VPolyB *pbase = &d;

				assert_not_equal(static_cast<void *>(&d), static_cast<void *>(pbase));
				assert_not_equal(sizeof(Bar), sizeof(Baz));
				setup_buffer();
			}

			void setup_buffer(size_t size = 1000, byte filling = 0)
			{
				buffer.assign(size, filling);
				begin = &buffer[0];
				end = begin + buffer.size();
				p = begin;
			}

			test( EntryCreateConstructsValueObject )
			{
				// INIT
				typedef pair<int, string> pair1_t;
				typedef pair<string, double> pair2_t;

				// ACT
				byte *v1 = p;
				poly_entry<pair1_t>::push(p, begin, end, pair1_t(171, "zee"));
				AUTO_DESTROY(poly_entry<pair1_t>, v1, begin, end);

				// ASSERT
				assert_is_true(p > v1);

				// ACT / ASSERT
				assert_equal(171, poly_entry<pair1_t>::adjust_get(v1, begin, end).first);
				assert_equal("zee", poly_entry<pair1_t>::adjust_get(v1, begin, end).second);

				// ACT
				byte *v2 = p;
				poly_entry<pair2_t>::push(p, begin, end, pair2_t("Lorem ipsum", 3.14159));
				AUTO_DESTROY(poly_entry<pair2_t>, v2, begin, end);

				// ASSERT
				assert_is_true(p > v2);

				// ACT / ASSERT
				assert_equal("Lorem ipsum", poly_entry<pair2_t>::adjust_get(v2, begin, end).first);
				assert_equal(3.14159, poly_entry<pair2_t>::adjust_get(v2, begin, end).second);
			}


			test( PolymorphicalObjectsAreStoredPolymorphically )
			{
				// INIT
				int bars = 0, bazes = 0;

				// ACT
				byte *v1 = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", bars));
				AUTO_DESTROY(poly_entry<Foo>, v1, begin, end);
				byte *v2 = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Chekhov", bars));
				AUTO_DESTROY(poly_entry<Foo>, v2, begin, end);
				byte *v3 = p;
				poly_entry<Foo>::push(p, begin, end, Baz(13, bazes));
				AUTO_DESTROY(poly_entry<Foo>, v3, begin, end);
				byte *v4 = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Brodsky", bars));
				AUTO_DESTROY(poly_entry<Foo>, v4, begin, end);

				// ACT / ASSERT
				assert_equal(3, bars);
				assert_equal(1, bazes);
				assert_equal("Bar", poly_entry<Foo>::adjust_get(v1, begin, end).get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(poly_entry<Foo>::adjust_get(v1, begin, end)).value);
				assert_equal("Bar", poly_entry<Foo>::adjust_get(v2, begin, end).get_type());
				assert_equal("Chekhov", static_cast<Bar&>(poly_entry<Foo>::adjust_get(v2, begin, end)).value);
				assert_equal("Baz", poly_entry<Foo>::adjust_get(v3, begin, end).get_type());
				assert_equal(13, static_cast<Baz&>(poly_entry<Foo>::adjust_get(v3, begin, end)).value);
				assert_equal("Bar", poly_entry<Foo>::adjust_get(v4, begin, end).get_type());
				assert_equal("Brodsky", static_cast<Bar&>(poly_entry<Foo>::adjust_get(v4, begin, end)).value);
			}


			test( DestroyingObjectsDestroysThemPolymorphically )
			{
				// INIT
				int bars = 0, bazes = 0;

				byte * const v1 = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", bars));
				poly_entry<Foo>::push(p, begin, end, Bar("Chekhov", bars));
				byte * const v3 = p;
				poly_entry<Foo>::push(p, begin, end, Baz(13, bazes));
				byte * const v4 = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Brodsky", bars));
				AUTO_DESTROY(poly_entry<Foo>, v4, begin, end);

				// ACT
				p = v3;
				poly_entry<Foo>::pop(p);

				// ACT / ASSERT
				assert_equal(3, bars);
				assert_equal(0, bazes);
				assert_equal(v4, p);

				// ACT
				p = v1;
				poly_entry<Foo>::pop(p);
				poly_entry<Foo>::pop(p);

				// ACT / ASSERT
				assert_equal(1, bars);
				assert_equal(0, bazes);
				assert_equal(v3, p);
			}


			test( EntryCreationWrapsPlacementIfInsufficientSpaceForDescriptor )
			{
				// INIT
				int dummy = 0;

				p = end - (sizeof(poly_entry_descriptor) - 1);

				// ACT
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", dummy));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT / ASSERT
				assert_is_true(p > begin);
				assert_is_true(p < end);
				assert_equal("Bar", poly_entry<Foo>::adjust_get(begin, begin, end).get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(poly_entry<Foo>::adjust_get(begin, begin, end)).value);
			}


			test( EntryCreationPlacesTerminatorAndWrapsIfInsufficientSpaceForValue1 )
			{
				// INIT
				int dummy = 0;

				setup_buffer(3 * (sizeof(poly_entry_descriptor) + sizeof(Bar)), 0xFF);
				p = end - (sizeof(poly_entry_descriptor) + sizeof(Bar) - 1);

				// ACT
				byte *terminator = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", dummy));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT / ASSERT
				poly_entry_descriptor *d = reinterpret_cast<poly_entry_descriptor *>(terminator);

				assert_is_true(p > begin);
				assert_is_true(p < end);
				assert_equal(0, d->size);
				assert_equal(0xFF, *(terminator + sizeof(poly_entry_descriptor)));
				assert_equal("Bar", poly_entry<Foo>::adjust_get(begin, begin, end).get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(poly_entry<Foo>::adjust_get(begin, begin, end)).value);
			}


			test( EntryCreationPlacesTerminatorAndWrapsIfInsufficientSpaceForValue2 )
			{
				// INIT
				int dummy = 0;

				setup_buffer(100, 0xFF);
				p = end - (sizeof(poly_entry_descriptor) + sizeof(Baz) - 1);

				// ACT
				byte *terminator = p;
				poly_entry<Foo>::push(p, begin, end, Baz(19, dummy));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT / ASSERT
				poly_entry_descriptor *d = reinterpret_cast<poly_entry_descriptor *>(terminator);

				assert_is_true(p > begin);
				assert_is_true(p < end);
				assert_equal(0, d->size);
				assert_equal(0xFF, *(terminator + sizeof(poly_entry_descriptor)));
				assert_equal("Baz", poly_entry<Foo>::adjust_get(begin, begin, end).get_type());
				assert_equal(19, static_cast<Baz&>(poly_entry<Foo>::adjust_get(begin, begin, end)).value);
			}


			test( EntryCreationPlacesTerminatorAndWrapsIfInsufficientSpaceForValue3 )
			{
				// INIT
				int dummy = 0;

				setup_buffer(100, 0xFF);
				p = end - sizeof(poly_entry_descriptor);

				// ACT
				byte *terminator = p;
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", dummy));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT / ASSERT
				poly_entry_descriptor *d = reinterpret_cast<poly_entry_descriptor *>(terminator);

				assert_is_true(p > begin);
				assert_is_true(p < end);
				assert_equal(0, d->size);
				assert_equal("Bar", poly_entry<Foo>::adjust_get(begin, begin, end).get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(poly_entry<Foo>::adjust_get(begin, begin, end)).value);
			}


			test( ElementIsReadAtTheBeginingIfInsufficientSpaceForDescriptorToRead )
			{
				// INIT
				int count = 0;
				byte *v1 = end - (sizeof(poly_entry_descriptor) - 1);

				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", count));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT
				Foo &object = poly_entry<Foo>::adjust_get(v1, begin, end);

				// ASSERT
				assert_equal(begin, v1);
				assert_equal("Bar", object.get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(object).value);
			}


			test( ElementIsReadAtTheBeginingIfNoSpaceForDescriptorToRead )
			{
				// INIT
				int count = 0;
				byte *v1 = end;

				poly_entry<Foo>::push(p, begin, end, Baz(1984, count));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT
				Foo &object = poly_entry<Foo>::adjust_get(v1, begin, end);

				// ASSERT
				assert_equal(begin, v1);
				assert_equal("Baz", object.get_type());
				assert_equal(1984, static_cast<Baz&>(object).value);
			}


			test( ElementIsReadAtTheBeginingIfTerminatorIsMet )
			{
				// INIT
				int count = 0;
				poly_entry_descriptor zero = { };
				byte *v1 = end - (sizeof(poly_entry_descriptor));

				*reinterpret_cast<poly_entry_descriptor *>(v1) = zero;
				poly_entry<Foo>::push(p, begin, end, Bar("Tolstoy", count));
				AUTO_DESTROY(poly_entry<Foo>, begin, begin, end);

				// ACT
				Foo &object = poly_entry<Foo>::adjust_get(v1, begin, end);

				// ASSERT
				assert_equal(begin, v1);
				assert_equal("Bar", object.get_type());
				assert_equal("Tolstoy", static_cast<Bar&>(object).value);
			}


			test( GetWorksWellWhenBaseAndDerivedAreDisplaced )
			{
				// INIT
				byte *v1 = p;
				poly_entry<VPolyB>::push(p, begin, end, VDerived());
				AUTO_DESTROY(poly_entry<VPolyB>, v1, begin, end);
				byte *v2 = p;
				poly_entry<VPolyB>::push(p, begin, end, VDerived());
				AUTO_DESTROY(poly_entry<VPolyB>, v2, begin, end);
				byte *v3 = p;
				poly_entry<VPolyB>::push(p, begin, end, VDerived());
				AUTO_DESTROY(poly_entry<VPolyB>, v3, begin, end);

				// ACT / ASSERT
				assert_equal("derived", poly_entry<VPolyB>::adjust_get(v1, begin, end).get_type2());
				assert_equal("derived", poly_entry<VPolyB>::adjust_get(v2, begin, end).get_type2());
				assert_equal("derived", poly_entry<VPolyB>::adjust_get(v3, begin, end).get_type2());
			}


			test( EntryCanBeCreatedByMoving )
			{
				// INIT
				int n = 0;
				BazNC o1(123, n), o2(1311, n);

				// ACT
				byte *v1 = p;
				poly_entry<Foo>::push(p, begin, end, move(o1));
				AUTO_DESTROY(poly_entry<Foo>, v1, begin, end);
				byte *v2 = p;
				poly_entry<Foo>::push(p, begin, end, move(o2));
				AUTO_DESTROY(poly_entry<Foo>, v2, begin, end);

				// ASSERT
				assert_equal(2, n);
				assert_equal(-1, o1.value);
				assert_equal(-1, o2.value);
				assert_equal("BazNC", poly_entry<Foo>::adjust_get(v1, begin, end).get_type());
				assert_equal(123, static_cast<BazNC &>(poly_entry<Foo>::adjust_get(v1, begin, end)).value);
				assert_equal("BazNC", poly_entry<Foo>::adjust_get(v2, begin, end).get_type());
				assert_equal(1311, static_cast<BazNC &>(poly_entry<Foo>::adjust_get(v2, begin, end)).value);
			}


			test( CopyConstructedObjectsFromNonConstRefAreHeldInTheSameWayAsConstConstructed )
			{
				// INIT
				int n = 0;
				BazNC o1(123, n), o2(1311, n);

				// INIT / ACT
				byte *v1 = p;
				poly_entry<Foo>::push(p, begin, end, move(o1));
				byte *v2 = p;
				poly_entry<Foo>::push(p, begin, end, move(o2));

				// ACT
				poly_entry<Foo>::pop(v1);

				// ASSERT
				assert_equal(v2, v1);
				assert_equal(1, n);

				// ACT
				poly_entry<Foo>::pop(v2);

				// ASSERT
				assert_equal(p, v2);
				assert_equal(0, n);
			}

		end_test_suite
	}
}
