#include "ml_mmap.h"
#include "ml_macros.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

ML_TYPE(MLMMapT, (MLAddressT), "mmap");

ML_TYPE(MLMMapBufferT, (MLMMapT, MLBufferT), "mmap::buffer");

ML_METHOD(MLMMapT, MLStringT, MLStringT) {
	int OpenMode, MMapProtect;
	ml_type_t *Type;
	switch (ml_string_value(Args[1])[0]) {
	case 'r':
		OpenMode = O_RDONLY;
		MMapProtect = PROT_READ;
		Type = MLMMapT;
		break;
	case 'w':
		OpenMode = O_RDWR | O_CREAT;
		MMapProtect = PROT_READ | PROT_WRITE;
		Type = MLMMapBufferT;
		break;
	default:
		return ml_error("ValueError", "Invalid mode for mmap");
	}
	const char *Path = ml_string_value(Args[0]);
	int Fd = open(ml_string_value(Args[0]), OpenMode, 0600);
	if (Fd < 0) return ml_error("FileError", "failed to map %s in mode %s: %s", Path, ml_string_value(Args[1]), strerror(errno));
	struct stat Stat[1];
	fstat(Fd, Stat);
	void *Ptr = mmap(NULL, Stat->st_size, MMapProtect, MAP_SHARED, Fd, 0);
	if (Ptr == MAP_FAILED) {
		close(Fd);
		return ml_error("MMapError", "failed to map %s in mode %s: %s", Path, ml_string_value(Args[1]), strerror(errno));
	}
	close(Fd);
	ml_address_t *MMap = (ml_address_t *)ml_address(Ptr, Stat->st_size);
	MMap->Type = Type;
	return (ml_value_t *)MMap;
}

ML_METHOD("unmap", MLMMapT) {
	ml_address_t *MMap = (ml_address_t *)Args[0];
	if (munmap(MMap->Value, MMap->Length) < 0) {
		return ml_error("MMapError", "failed to unmap: %s", strerror(errno));
	}
	return MLNil;
}

void ml_mmap_init(stringmap_t *Globals) {
#include "ml_mmap_init.c"
	stringmap_insert(MLMMapT->Exports, "buffer", MLMMapBufferT);
	if (Globals) {
		stringmap_insert(Globals, "mmap", MLMMapT);
	}
}
