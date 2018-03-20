# AlbumFS

## A key encrypted LSB steganography PNG album filesystem in user space for Linux

Create, access, and modify a key encrypted LSB steganography filesystem in user space using a directory of PNG images.  Filesystem state is only preserved after safely closing the filesystem via unmount or Ctrl+C.  A filesystem may only use images with the same dimensions as the root image provided.

A filesystem requires a name, key, root image, and storage images.  To access a filesystem the correct key, filesystem name, and root image must be given.  All data is XOR'd with the SHA512 hash of the key as it is read and written from the images.  The root image stores the filesystem name, consumed and total space, image and file count, image hashs, and file meta data.  All images added to the filesystem while formatting or expanding are found in the same directory as the root image.

Formatting a filesystem wipes each available least significant bit in the images provided, similarly removing a file wipes its data and shifts the filesystem if there is a hole.  All files in the filesystem have permissions of 644 and cannot be edited, but can be read, renamed, deleted, and copied.

The [wiki](https://github.com/mgeitz/albumfs/wiki) contains an implementation summary and some additional details.

### Dependencies
* libfuse-dev
* libpng-dev
* libssl-dev
* pkg-config


### Compile
* make all
* sudo make install


### Commands

| Command	| Description	|
|---------------|---------------|
| -format	| create a new filesystem using valid images in directory of root image provided and mount it.|
| -expand	| expand  and mount an existing filesystem. The directory containing the root image is scanned for additional valid PNG images not already included in the filesystem.|
| -mount	| mount an existing filesystem|
| -debug	| enable bit by bit debug output. IO blocking from this output will cause the filesystem to operate very slowly.|
| -help		| echo command syntax and exit program.|


### Examples
```sh
$ albumfs -format images/vacation/image.png
```
```sh
$ albumfs -debug -mount images/vacation/image.png
```
```sh
$ man albumfs
```

### Contributing

* **Fork** the repo
* **Clone** the repo locally
* **Checkout** the `develop` branch
* **Commit** your change [accordingly](https://github.com/torvalds/subsurface-for-dirk/blob/master/README#L92)
* **Push** your change to your remote fork
* Submit a **Pull request** against the develop branch
