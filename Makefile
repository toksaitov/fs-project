CFLAGS += -g -std=gnu99 -MMD -D_FILE_OFFSET_BITS=64

FUSE_PKG := $(shell pkg-config --exists fuse 2>/dev/null && echo fuse || echo fuse-t)
FUSE_CFLAGS += `pkg-config $(FUSE_PKG) --cflags` -DFUSE_USE_VERSION=26
FUSE_LDLIBS += `pkg-config $(FUSE_PKG) --libs`

# KRFFS

KRFFS_TARGET  = krffs
KRFFS_OBJECTS = krffs_file_system.o     \
                krffs_node.o            \
                krffs_allocator.o       \
                krffs_platform.o        \
                krffs_utilities.o       \
                krffs_fuse_operations.o \
                krffs.o
KRFFS_DEPENDENCIES = $(KRFFS_OBJECTS:%.o=%.d)

# mkfs.krffs

MKFS_KRFFS_TARGET  = mkfs.krffs
MKFS_KRFFS_OBJECTS = krffs_file_system.o \
                     krffs_node.o        \
                     krffs_platform.o    \
                     krffs_utilities.o   \
                     mkfs.krffs.o
MKFS_KRFFS_DEPENDENCIES = $(MKFS_KRFFS_OBJECTS:%.o=%.d)

# defrag.krffs

DEFRAG_KRFFS_TARGET  = defrag.krffs
DEFRAG_KRFFS_OBJECTS = krffs_file_system.o \
                       krffs_node.o        \
                       krffs_platform.o    \
                       krffs_utilities.o   \
                       defrag.krffs.o
DEFRAG_KRFFS_DEPENDENCIES = $(DEFRAG_KRFFS_OBJECTS:%.o=%.d)

# fsck.krffs

FSCK_KRFFS_TARGET  = fsck.krffs
FSCK_KRFFS_OBJECTS = krffs_file_system.o \
                     krffs_node.o        \
                     krffs_platform.o    \
                     krffs_utilities.o   \
                     fsck.krffs.o
FSCK_KRFFS_DEPENDENCIES = $(FSCK_KRFFS_OBJECTS:%.o=%.d)

# fsviz.krffs

FSVIZ_KRFFS_TARGET  = fsviz.krffs
FSVIZ_KRFFS_OBJECTS = krffs_file_system.o \
                      krffs_node.o        \
                      krffs_platform.o    \
                      krffs_utilities.o   \
                      fsviz.krffs.o
FSVIZ_KRFFS_DEPENDENCIES = $(FSVIZ_KRFFS_OBJECTS:%.o=%.d)

# edfs.krffs

EDFS_KRFFS_TARGET  = edfs.krffs
EDFS_KRFFS_OBJECTS = krffs_file_system.o \
                     krffs_node.o        \
                     krffs_platform.o    \
                     krffs_utilities.o   \
                     edfs.krffs.o
EDFS_KRFFS_DEPENDENCIES = $(EDFS_KRFFS_OBJECTS:%.o=%.d)

# ---

TARGETS = $(KRFFS_TARGET)        \
          $(MKFS_KRFFS_TARGET)   \
          $(DEFRAG_KRFFS_TARGET) \
          $(FSCK_KRFFS_TARGET)   \
          $(FSVIZ_KRFFS_TARGET)  \
          $(EDFS_KRFFS_TARGET)
OBJECTS = $(KRFFS_OBJECTS)        \
          $(MKFS_KRFFS_OBJECTS)   \
          $(DEFRAG_KRFFS_OBJECTS) \
          $(FSCK_KRFFS_OBJECTS)   \
          $(FSVIZ_KRFFS_OBJECTS)  \
          $(EDFS_KRFFS_OBJECTS)
DEPENDENCIES = $(KRFFS_DEPENDENCIES)        \
               $(MKFS_KRFFS_DEPENDENCIES)   \
               $(DEFRAG_KRFFS_DEPENDENCIES) \
               $(FSCK_KRFFS_DEPENDENCIES)   \
               $(FSVIZ_KRFFS_DEPENDENCIES)  \
               $(EDFS_KRFFS_DEPENDENCIES)

.PHONY : all
all : $(TARGETS)

# KRFFS

$(KRFFS_TARGET) : $(KRFFS_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS) $(FUSE_LDLIBS)

$(KRFFS_OBJECTS) : %.o : %.c
	$(CC) $(CFLAGS) $(FUSE_CFLAGS) -c $< -o $@

# mkfs.krffs

$(MKFS_KRFFS_TARGET) : $(MKFS_KRFFS_OBJECTS)

$(MKFS_KRFFS_OBJECTS) : %.o : %.c

# defrag.krffs

$(DEFRAG_KRFFS_TARGET) : $(DEFRAG_KRFFS_OBJECTS)

$(DEFRAG_KRFFS_OBJECTS) : %.o : %.c

# fsck.krffs

$(FSCK_KRFFS_TARGET) : $(FSCK_KRFFS_OBJECTS)

$(FSCK_KRFFS_OBJECTS) : %.o : %.c

# fsviz.krffs

$(FSVIZ_KRFFS_TARGET) : $(FSVIZ_KRFFS_OBJECTS)

$(FSVIZ_KRFFS_OBJECTS) : %.o : %.c

# edfs.krffs

$(EDFS_KRFFS_TARGET) : $(EDFS_KRFFS_OBJECTS)

$(EDFS_KRFFS_OBJECTS) : %.o : %.c

# ---

-include $(DEPENDENCIES)

.PHONY : clean
clean :
	rm -f $(TARGETS) $(OBJECTS) $(DEPENDENCIES)
