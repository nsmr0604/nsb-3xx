Linux Kernel Driver package for SPAcc/SPAcc-PDU with support for PKA, TRNG, RE, EA, KEP, CLP-30/36, and MPM modules
Copyright (c) 2013, Elliptic Technologies Inc.
Packaged: 15 July 2013

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

This package includes Linux 3.6.7+ drivers for the Elliptic CLP-30, SPAcc, SPAcc-PDU, CLP-27, CLP-800, and PKA
related devices.  The drivers were tested on little endian 32-bit hosts and should be portable to both
x86 and ARM platforms.

The package has the following directory structure:

elliptic
|-- doc                       <--- SDK Documentation (EDN-0510 located here)
`-- driver
    |-- bin                   <--- Destination for compiled libraries, applications, and kernel modules
    |-- patches
    `-- src
        |-- clp30             <--- CLP-30/36 SDK
        |   |-- clp30
        |   |-- include
        |   `-- kernel        <--- Linux Kernel modules and integration
        |-- core              <--- SPAcc SDK
        |   |-- include            [*]
        |   |-- kernel        <--- Linux Kernel modules and integration
        |   |-- spacc         <--- Portable SPAcc control SDK 
        |   `-- user          <--- Userspace library [*] for Linux userspace driver
        |-- ea                <--- SPAcc-EA (SPP-230/330) IPsec SDK
        |   |-- ea            <--- Portable EA control SDK
        |   |-- include
        |   `-- kernel        <--- Linux Kernel modules and integration
        |-- include
        |   `-- crypto
        |-- mpm               <--- SPAcc-MPM SDK
        |   |-- include
        |   |-- kernel        <--- Linux Kernel modules
        |   `-- mpm           <--- Portable MPM control SDK
        |-- pdu               <--- Platform Device SDK [*]
        |   |-- bare          <--- Bare metal platform template
        |   |   |-- include
        |   |   |-- kernel
        |   |   `-- pdu
        |   `-- linux         <--- Linux platform template
        |       |-- include
        |       |-- kernel
        |       `-- pdu
        |-- pka               <--- PKA SDK
        |   |-- include
        |   |-- kernel        <--- Linux Kernel modules
        |   |-- pka           <--- Portable PKA control SDK
        |   `-- user          <--- Userspace Library [*]
        |       |-- diag
        |       `-- engine
        |           |-- m4
        |           `-- src
        |-- re                <--- SPAcc-RE Record Engine SDK
        |   |-- include            [*]
        |   |-- kep           <--- Portable KEP control SDK
        |   |-- kernel        <--- Linux Kernel modules for RE and KEP
        |   |-- re            <--- Portable RE control SDK
        |   `-- user          <--- Userspace Library [*]
        |-- saspa             <--- SASPA SDK
        |   |-- include
        |   |-- kernel        <--- Linux Kernel Modules
        |   `-- saspa         <--- Portable SASPA control SDK
        |-- test
        |   `-- offload
        `-- trng              <--- CLP-27 (TRNG) and CLP-800 (TRNG3) SDK
            |-- include
            |-- kernel        <--- Linux Kernel modules and integration
            |-- test
            |   |-- private
            |   |   `-- fta
            |   `-- trngdiag
            `-- trng          <--- Portable TRNG SDK


[*] Denotes directories containing code that be cross-licensed as non-GPL.

---- Building Drivers ----

The top level makefile will build all of the drivers and place them in elliptic/driver/bin by default.  Issuing
"make" is sufficient to build all of the drivers.  Minor changes may be required for cross compiling.

The modules names are

elppdu.ko        - The PDU platform_device module
elpspacc.ko      - Provides the SPAcc SDK in Kernel space (requires elppdu)
elpspaccusr.ko   - Provides userspace interface to the SPAcc (req: elpspacc, elppdu)
elpspacccrypt.ko - Provides Linux crypto API hash/cipher drivers.
elpre.ko         - Provides the RE SDK in Kernel space (req: elpspacc, elppdu)
elpreusr.ko      - Provides the /dev/spaccre interface to use the RE from userspace (req: elpre, elpspacc, elppdu)
elpkep.ko        - Provides the KEP SDK in Kernel space (req: elpspacc, elppdu)
elpkepdiag.ko    - Provides the KEP diagnostic interface /proc/kepdiag (req: elpkep, elpspacc, elppdu)
elpkepdev.ko     - Provides the /dev/spacckep interface to use the KEP from userspace (req: elpkep, elpspacc, elppdu)
elpmpm.ko        - Provides the MPM SDK in Kernel space (req: elpspacc, elppdu)
elpmpmdiag.ko    - Provides the MPM diagnostic interface /proc/mpmdiag (req: elpmpm, elpspacc, elppdu)
elpea.ko         - Provides the EA SDK in Kernel space (req: elpspacc, elppdu)
elpeaxfrm.ko     - Provides the EA Linux integration for IPSec (req: elpea, elpspacc, elppdu)
elptrng.ko       - Provides the CLP-27 (TRNG) SDK in Kernel space as well as creates a HW RNG device /dev/hwrng
elptrng3.ko      - Provides the CLP-800 (TRNG3) kernel interfaces and /dev/hwrng usage.

The "diag" modules are not required to be loaded for use of the drivers.  They are only provided to run vectors which would
be provided to customers on a per order basis.

---- Enabling test and debug features ----

Some of the drivers need to be built specially for testing or debugging.
To enable the test programs for the CLP-27, edit elliptic/driver/Makefile
and make sure that the following are set:

  PDU_OPTIONS += PDU_TEST_LEVEL=1
  PDU_OPTIONS += TRNG_DEBUG_API=1

and build the module with

  make

--- Using Crypto API ---

The Linux crypto API driver (elpspacccrypt.ko) is used to integrate the SPAcc
with other Linux subsystems (e.g., IPsec).  After registering the SPAcc
platform_device with the kernel, simply load the module to hook into the
cipher/hash systems.  There are, however, some limitations to note:

  * Due to some bugs/limitations with DMA mapping and crypto API in current
    versions of Linux, SG debugging must be disabled in the kernel config
    (CONFIG_DEBUG_SG=n).

  * Combined cipher/hash modes are not supported.  Linux will automatically
    convert such jobs into two separate cipher and hash operations.

  * For modes requiring separate encrypt/decrypt contexts (e.g., AES-ECB or
    AES-CBC), encrypt and decrypt must be done using separate handles.

  * The incremental ahash interface is not supported (i.e., crypto_ahash_init,
    crypto_ahash_update, crypto_ahash_final/finup).  Such calls will use a
    software fallback.  Only the crypto_ahash_digest function will use
    hardware (this is used by, for example, the AH drivers).
