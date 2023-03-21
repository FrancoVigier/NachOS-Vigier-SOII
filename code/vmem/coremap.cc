#include "coremap.hh"

Coremap::Coremap(unsigned int nitems)
{
	ASSERT(nitems > 0);
	framesMap = new Bitmap(nitems);
	addrSpaces = new AddressSpace*[nitems];
	vpns = new unsigned int[nitems];
	size = nitems;
}

Coremap::~Coremap()
{
	delete framesMap;
	delete [] addrSpaces;
	delete [] vpns;
}

unsigned int
Coremap::CountClear() const
{
	return framesMap->CountClear();
}

AddressSpace*
Coremap::GetAddressSpace(unsigned int frame)
{
	return Test(frame) ? addrSpaces[frame] : nullptr;
}

void
Coremap::Mark(unsigned int which, AddressSpace *space, unsigned int vpn)
{
	ASSERT(which >= 0 && which < size);

	framesMap->Mark(which);
	addrSpaces[which] = space;
	vpns[which] = vpn;
}

bool
Coremap::Test(unsigned int which) const
{
	ASSERT(which >= 0 && which < size);
	if(framesMap->Test(which)) {
		ASSERT(addrSpaces[which] != nullptr);
	}
	return framesMap->Test(which);
}

int
Coremap::Find(AddressSpace *space, unsigned int vpn)
{
	int which = framesMap->Find();
	if (which != -1) {
		addrSpaces[which] = space;
		vpns[which] = vpn;
	}

	return which;
}

void
Coremap::Clear(unsigned int which)
{
	ASSERT(which >= 0 && which < size);
	framesMap->Clear(which);
	addrSpaces[which] = nullptr;
}

unsigned int
Coremap::GetVpn(unsigned int frame)
{
	return Test(frame)? vpns[frame] : -1;
}