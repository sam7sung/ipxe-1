/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <gpxe/device.h>
#include <undi.h>
#include <undinet.h>
#include <undipreload.h>

/** @file
 *
 * "Pure" UNDI driver
 *
 * This is the UNDI driver without explicit support for PCI or any
 * other bus type.  It is capable only of using the preloaded UNDI
 * device.  It must not be combined in an image with any other
 * drivers.
 *
 * If you want a PXE-loadable image that contains only the UNDI
 * driver, build "bin/undionly.kpxe".
 *
 * If you want any other image format, or any other drivers in
 * addition to the UNDI driver, build e.g. "bin/undi.dsk".
 */

/**
 * Probe UNDI root bus
 *
 * @v rootdev		UNDI bus root device
 *
 * Scans the UNDI bus for devices and registers all devices it can
 * find.
 */
static int undibus_probe ( struct root_device *rootdev ) {
	struct undi_device *undi = &preloaded_undi;
	int rc;

	/* Check for a valie preloaded UNDI device */
	if ( ! undi->entry.segment ) {
		DBG ( "No preloaded UNDI device found!\n" );
		return -ENODEV;
	}

	/* Add to device hierarchy */
	strncpy ( undi->dev.name, "UNDI",
		  ( sizeof ( undi->dev.name ) - 1 ) );
	if ( undi->pci_busdevfn != UNDI_NO_PCI_BUSDEVFN ) {
		struct pci_device_description *pcidesc = &undi->dev.desc.pci;
		pcidesc->bus_type = BUS_TYPE_PCI;
		pcidesc->busdevfn = undi->pci_busdevfn;
		pcidesc->vendor = undi->pci_vendor;
		pcidesc->device = undi->pci_device;
	} else if ( undi->isapnp_csn != UNDI_NO_ISAPNP_CSN ) {
		struct isapnp_device_description *isapnpdesc
			= &undi->dev.desc.isapnp;
		isapnpdesc->bus_type = BUS_TYPE_ISAPNP;
	}
	undi->dev.parent = &rootdev->dev;
	list_add ( &undi->dev.siblings, &rootdev->dev.children);
	INIT_LIST_HEAD ( &undi->dev.children );

	/* Create network device */
	if ( ( rc = undinet_probe ( undi ) ) != 0 )
		goto err;

	return 0;

 err:
	list_del ( &undi->dev.siblings );
	return rc;
}

/**
 * Remove UNDI root bus
 *
 * @v rootdev		UNDI bus root device
 */
static void undibus_remove ( struct root_device *rootdev __unused ) {
	struct undi_device *undi = &preloaded_undi;

	undinet_remove ( undi );
	list_del ( &undi->dev.siblings );
}

/** UNDI bus root device driver */
static struct root_driver undi_root_driver = {
	.probe = undibus_probe,
	.remove = undibus_remove,
};

/** UNDI bus root device */
struct root_device undi_root_device __root_device = {
	.dev = { .name = "UNDI" },
	.driver = &undi_root_driver,
};
