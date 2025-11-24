CFLAGS += -g -std=gnu99 -MMD

FUSE_CFLAGS += `pkg-config fuse --cflags` -DFUSE_USE_VERSION=26
FUSE_LDLIBS += `pkg-config fuse --libs`
NCURSES_LDLIBS += `pkg-config ncurses --libs`

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

# MKFS.KRFFS

MKFS_KRFFS_TARGET  = mkfs.krffs
MKFS_KRFFS_OBJECTS = krffs_file_system.o \
                     krffs_node.o        \
                     krffs_platform.o    \
                     krffs_utilities.o   \
                     mkfs.krffs.o
MKFS_KRFFS_DEPENDENCIES = $(MKFS_KRFFS_OBJECTS:%.o=%.d)

# DEFRAG.KRFFS

DEFRAG_KRFFS_TARGET  = defrag.krffs
DEFRAG_KRFFS_OBJECTS = krffs_file_system.o \
                       krffs_node.o        \
                       krffs_platform.o    \
                       krffs_utilities.o   \
                       defrag.krffs.o
DEFRAG_KRFFS_DEPENDENCIES = $(DEFRAG_KRFFS_OBJECTS:%.o=%.d)

# FSCK.KRFFS

FSCK_KRFFS_TARGET  = fsck.krffs
FSCK_KRFFS_OBJECTS = krffs_file_system.o \
                     krffs_node.o        \
                     krffs_platform.o    \
                     krffs_utilities.o   \
                     fsck.krffs.o
FSCK_KRFFS_DEPENDENCIES = $(FSCK_KRFFS_OBJECTS:%.o=%.d)

# FSVIZ.KRFFS

FSVIZ_KRFFS_TARGET  = fsviz.krffs
FSVIZ_KRFFS_OBJECTS = krffs_file_system.o \
                      krffs_node.o        \
                      krffs_platform.o    \
                      krffs_utilities.o   \
                      fsviz.krffs.o
FSVIZ_KRFFS_DEPENDENCIES = $(FSVIZ_KRFFS_OBJECTS:%.o=%.d)

# EDFS.KRFFS

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

# MKFS.KRFFS

$(MKFS_KRFFS_TARGET) : $(MKFS_KRFFS_OBJECTS)

$(MKFS_KRFFS_OBJECTS) : %.o : %.c

# DEFRAG.KRFFS

$(DEFRAG_KRFFS_TARGET) : $(DEFRAG_KRFFS_OBJECTS)

$(DEFRAG_KRFFS_OBJECTS) : %.o : %.c

# FSCK.KRFFS

$(FSCK_KRFFS_TARGET) : $(FSCK_KRFFS_OBJECTS)

$(FSCK_KRFFS_OBJECTS) : %.o : %.c

# FSVIZ.KRFFS

$(FSVIZ_KRFFS_TARGET) : $(FSVIZ_KRFFS_OBJECTS)
	$(CC) -g -O0 $(LDFLAGS) -o $@ $^ $(LDLIBS) $(NCURSES_LDLIBS)

$(FSVIZ_KRFFS_OBJECTS) : %.o : %.c

# EDFS.KRFFS

$(EDFS_KRFFS_TARGET) : $(EDFS_KRFFS_OBJECTS)

$(EDFS_KRFFS_OBJECTS) : %.o : %.c

# ---

-include $(DEPENDENCIES)

.PHONY : clean
clean :
	rm -f $(TARGETS) $(OBJECTS) $(DEPENDENCIES)
