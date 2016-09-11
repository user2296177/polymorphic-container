###`polymorphic_vector<B>`, a contiguous storage container for type polymorphism. 

Normally, in order to have a polymorphic collection, we store pointers to a base class.

Example:

    std::vector<base*> v;          // std::vector<std::unique_ptr<base>> v;
    v.push_back( new derived{} );  // v.push_back( new derived{} );

There are a couple of issues with this approach:

 - The `std::vector<T>` instance does not manage the lifetime of the polymorphic types. They must be allocated elsewhere.
 - The instances are not allocated contiguously; there are no standard library facilities that provide contiguous storage for any type in an inhertiance hierarchy.

Ideally, one would be able to simply declare the following:

    polymorphic_vector<base> v;
    v.push_back( derived_a{}  ); // some type that derives from base; ensured through templates
    v.push_back( derived_b{}  ); // another type that derives from base, might have a different size than derived_a
    
Such a type would have the following assurances:

 - It will be able to store any type that derives from the specified base class. Including alignment and size.
 - New derived types are automatically supported by the container. Including alignment and size.
 - Values that are added to the container are stored in contiguous memory.

Here is a sample usage program example:

    #include <iostream>
    #include <string>
    #include <vector>

    struct base
    {
	    virtual ~base() = default;
	    virtual void print() const = 0;
    };

    struct derived_a : public base
    {
	    derived_a( std::string&& s ) noexcept : s_{ std::move( s ) } {}

	    void print() const override
	    {
		    std::cout << s_ << '\n';
	    }

	    std::string s_;
    };

    struct derived_b : public base
    {
    	derived_b( std::vector<int>&& v ) noexcept : v_{ std::move( v ) } {}
	
	    void print() const override
	    {
		    for ( std::vector<int>::size_type i{ 0 }, size{ v_.size() }; i != size; ++i )
		    {
			    std::cout << v_[ i ];
		    }
		    std::cout << '\n';
	    }
	
	    std::vector<int> v_;
    };

    int main()
    {
	    gut::polymorphic_vector<base> pv;
	
	    pv.push_back( derived_a{ "abc" } );
	    pv.push_back( derived_b{ std::vector<int>{ 0, 1, 2, 3 } } );
	    pv.emplace_back<derived_b>( std::vector<int>{ 4, 4, 5, 5, 6, 6, 7, 8, 9 } );
	    pv.emplace_back<derived_a>( "defghijklmno" );

	    for ( base const& base_ref : pv )
		    base_ref.print();
    }
    
Here are some issues to consider sorrounding such storage:

 - Storing types of different sizes and alignments in contiguous storage.
 - Calling the proper member functions (constructors, destructor, etc.) when required.
 - Iterating through the collection of differently sized derived values.
 - Erasing and shifting differently sized derived values inside contiguous storage.
 
This implementation is intended to solve those and other issues that occur when requiring such a container.
