#ifndef NACHOS_LIB_COREMAP__HH
#define NACHOS_LIB_COREMAP__HH


#include "lib/bitmap.hh"
#include "userprog/address_space.hh"

class Coremap {
public:

    /// Initialize a coremap with `nitems` bits; all bits are cleared.
    ///
    /// * `nitems` is the number of items in the bitmap.
    Coremap(unsigned int nitems);

    /// Uninitialize a coremap.
    ~Coremap();

	/// Return the number of clear bits.
	unsigned int CountClear() const;

	AddressSpace *GetAddressSpace(unsigned int frame);

    /// Set the “nth” bit.
    void Mark(unsigned int which, AddressSpace *space, unsigned int vpn);

    /// Clear the “nth” bit.
    void Clear(unsigned int which);

    /// Is the “nth” bit set?
    bool Test(unsigned int which) const;

    /// Given an address space and a virtual page, finds a physical frame.
    /// If a frame can't be found it returns -1.
    int Find(AddressSpace *space, unsigned int vpn);

	unsigned int GetVpn(unsigned int frame);
    
private:
	unsigned int size;
	Bitmap *framesMap;
	AddressSpace **addrSpaces;
	unsigned int *vpns;
};


#endif