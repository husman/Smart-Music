/*
 ============================================================================
 Name        : usb-skeleton.c
 Author      : Haleeq Usman
 Version     : 0.0
 Description : Smart Music Driver (dev)
 ============================================================================
 */

//#include <linux/config.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kref.h>
#include <linux/smp.h>
#include <linux/usb.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>


/* Define these values to match your devices */
#define USB_SKEL_VENDOR_ID			0x07cf
#define USB_SKEL_PRODUCT_ID_HALEEQ	0x6803
#define USB_SKEL_PRODUCT_ID_MUKESH	0x6802

/* table of devices that work with this driver */
static struct usb_device_id skel_table [] = {
	/*{ USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID_HALEEQ) },
	{ USB_DEVICE(USB_SKEL_VENDOR_ID, USB_SKEL_PRODUCT_ID_MUKESH) },*/
	{ .driver_info = 42 },
	{ }					/* Terminating entry */
};
MODULE_DEVICE_TABLE (usb, skel_table);


/* Get a minor range for your devices from the usb maintainer */
#define USB_SKEL_MINOR_BASE	192

/* Structure to hold all of our device specific stuff */
struct usb_skel {
	struct usb_device *	udev;			/* the usb device for this device */
	struct usb_interface *	interface;		/* the interface for this device */
	unsigned char *		bulk_in_buffer;		/* the buffer to receive data */
	size_t			bulk_in_size;		/* the size of the receive buffer */
	unsigned char *		int_in_buffer;		/* the buffer to receive data */
	size_t			int_in_size;		/* the size of the receive buffer */
	__u8			bulk_in_endpointAddr;	/* the address of the bulk in endpoint */
	__u8			bulk_out_endpointAddr;	/* the address of the bulk out endpoint */
	__u8			int_in_endpointAddr;	/* the address of the interrupt in endpoint */
	
	/*
	 * Endpoint intervals
	 */
	__u8  int_in_endpointInterval;
	__u8  bulk_in_endpointInterval;
	__u8  bulk_out_endpointInterval;
	
	/*
	 * URBs
	 */
	struct urb * int_in_urb;
	    
	unsigned int	irqN;
	struct kref		kref;
};
#define to_skel_dev(d) container_of(d, struct usb_skel, kref)

static struct usb_driver skel_driver;

static void skel_delete(struct kref *kref)
{	
	struct usb_skel *dev = to_skel_dev(kref);

	usb_put_dev(dev->udev);
	kfree (dev->bulk_in_buffer);
	kfree (dev);
}

static int skel_open(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;
	struct usb_interface *interface;
	int subminor;
	int retval = 0;

	subminor = iminor(inode);

	interface = usb_find_interface(&skel_driver, subminor);
	if (!interface) {
		err ("%s - error, can't find device for minor %d",
		     __FUNCTION__, subminor);
		retval = -ENODEV;
		goto exit;
	}
	printk(KERN_DEBUG "[HALEEQ PIANO]----> OPENED...  USB device...\n");

	dev = usb_get_intfdata(interface);
	if (!dev) {
		retval = -ENODEV;
		goto exit;
	}
	
	/* increment our usage count for the device */
	kref_get(&dev->kref);

	/* save our object in the file's private structure */
	file->private_data = dev;

exit:
	return retval;
}

static int skel_release(struct inode *inode, struct file *file)
{
	struct usb_skel *dev;

	dev = (struct usb_skel *)file->private_data;
	if (dev == NULL)
		return -ENODEV;

	/* decrement the count on our device */
	kref_put(&dev->kref, skel_delete);
	return 0;
}

static ssize_t skel_read(struct file *file, char __user *buffer, size_t count, loff_t *ppos)
{
	struct usb_skel *dev;
	int retval = 0;

	dev = (struct usb_skel *)file->private_data;
	
	/* do a blocking bulk read to get data from the device */
	retval = usb_bulk_msg(dev->udev,
			      usb_rcvbulkpipe(dev->udev, dev->bulk_in_endpointAddr),
			      dev->bulk_in_buffer,
			      min(dev->bulk_in_size, count),
			      &count, HZ*10);

	/* if the read was successful, copy the data to userspace */
	if (!retval) {
		if (copy_to_user(buffer, dev->bulk_in_buffer, count))
			retval = -EFAULT;
		else
			retval = count;
	}
	printk(KERN_DEBUG "[HALEEQ PIANO]----> READ USB: %0X %0X %0X %0X\n", dev->bulk_in_buffer[0]
	                                                      , dev->bulk_in_buffer[1]
	                                                      , dev->bulk_in_buffer[2]
	                                                      , dev->bulk_in_buffer[3]);

	return retval;
}

static void skel_write_bulk_callback(struct urb *urb, struct pt_regs *regs)
{
	/* sync/async unlink faults aren't errors */
	if (urb->status && 
	    !(urb->status == -ENOENT || 
	      urb->status == -ECONNRESET ||
	      urb->status == -ESHUTDOWN)) {
		dbg("%s - nonzero write bulk status received: %d",
		    __FUNCTION__, urb->status);
	}
	
	printk(KERN_DEBUG "[HALEEQ PIANO]: ----CALLBACK--- transfer_buffer = %c\n", &(urb->transfer_buffer));

	/* free up our allocated buffer */
	usb_free_coherent(urb->dev, urb->transfer_buffer_length, 
			urb->transfer_buffer, urb->transfer_dma);
}

static ssize_t skel_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *ppos)
{
	struct usb_skel *dev;
	int retval = 0;
	struct urb *urb = NULL;
	char *buf = NULL;
	char buf2[1];

	dev = (struct usb_skel *)file->private_data;

	/* verify that we actually have some data to write */
	if (count == 0)
		goto exit;

	/* create a urb, and a buffer for it, and copy the data to the urb */
	urb = usb_alloc_urb(0, GFP_KERNEL);
	if (!urb) {
		retval = -ENOMEM;
		goto error;
	}

	buf = usb_alloc_coherent(dev->udev, count, GFP_KERNEL, &urb->transfer_dma);
	if (!buf) {
		retval = -ENOMEM;
		goto error;
	}
	if (copy_from_user(buf, user_buffer, count)) {
		retval = -EFAULT;
		goto error;
	}

	/*int zz = usb_control_msg(dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			0xD8, 0x41,
                      	0x00, 0x00,
                      	buf, count, 200 * HZ);
	printk(KERN_DEBUG "[HALEEQ PIANO]: ----WRITE: usb_control_msg() returns: %d\n", buf, zz);
	zz = usb_control_msg(dev->udev,
			usb_sndctrlpipe(dev->udev, 0),
			0xDB, 0x41,
                      	0x00, 0x00,
                      	buf, count, 200 * HZ);
	printk(KERN_DEBUG "[HALEEQ PIANO]: ----WRITE: usb_control_msg() returns: %d\n", buf, zz);
	unsigned char *mbuf = kmalloc(sizeof(unsigned char), GFP_KERNEL);
	zz = usb_control_msg(dev->udev,
			usb_rcvctrlpipe(dev->udev, 0),
			0xD6, 0xC0,
			0x00, 0x00,
			mbuf, sizeof(unsigned char), 200 * HZ);
	printk(KERN_DEBUG "[HALEEQ PIANO]: ----READ COMMAND: value: %d usb_control_msg() returns: %d\n", *mbuf, zz);
	*/
	char *buf3 = kmalloc(1, GFP_KERNEL);
	unsigned char *spk = kmalloc(1, GFP_KERNEL);
	spk[0] = 0xD6;
	/* initialize the urb properly */
	usb_fill_bulk_urb(urb, dev->udev,
			  usb_sndbulkpipe(dev->udev, dev->bulk_out_endpointAddr),
			  buf, count, skel_write_bulk_callback, dev);
	/*usb_fill_control_urb(urb, dev->udev,
			  usb_sndctrlpipe(dev->udev, 0),
			  spk, spk, 1, skel_write_bulk_callback, dev);*/
	urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;

	// send the data out the bulk port 
	retval = usb_submit_urb(urb, GFP_KERNEL);
	if (retval) {
		err("%s - failed submitting write urb, error %d", __FUNCTION__, retval);
		goto error;
	}

	printk(KERN_DEBUG "[HALEEQ]: ----SUbmitted urb---> returns: %d --> spk = %c\n", retval, *spk);

	// release our reference to this urb, the USB core will eventually free it entirely
	usb_free_urb(urb);

exit:
	return count;

error:
	usb_free_coherent(dev->udev, count, buf, urb->transfer_dma);
	usb_free_urb(urb);
	kfree(buf);
	return retval;
}

static struct file_operations skel_fops = {
	.owner =	THIS_MODULE,
	.read =		skel_read,
	.write =	skel_write,
	.open =		skel_open,
	.release =	skel_release,
};

/* 
 * usb class driver info in order to get a minor number from the usb core,
 * and to have the device registered with devfs and the driver core
 */
static struct usb_class_driver skel_class = {
	.name = "usb/skel%d",
	.fops = &skel_fops,
	//.mode = S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH,
	.minor_base = USB_SKEL_MINOR_BASE,
};

static void interrupt_handler(struct urb * urb)
{
	int retval = usb_submit_urb(urb, GFP_ATOMIC);
	if(retval != 0) {
		printk(KERN_DEBUG "[HALEEQ PIANO ERROR]: %s - error %d submitting interrupt urb\n", __FUNCTION__, retval);
	}
	printk(KERN_DEBUG "[HALEEQ PIANO IRQ HANDLER]: CALLED IRQ HANDLER!\n");
}
static int skel_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	printk(KERN_DEBUG "[HALEEQ PIANO DEVICE CLASS]: PROBE CALLED\n");
	struct usb_skel *dev = NULL;
	struct usb_host_interface *iface_desc;
	struct usb_endpoint_descriptor *endpoint;
	size_t buffer_size;
	int i;
	int retval = -ENOMEM;

	/* allocate memory for our device state and initialize it */
	dev = kmalloc(sizeof(struct usb_skel), GFP_KERNEL);
	if (dev == NULL) {
		err("Out of memory");
		goto error;
	}
	memset(dev, 0x00, sizeof (*dev));
	kref_init(&dev->kref);

	dev->udev = usb_get_dev(interface_to_usbdev(interface));
	dev->interface = interface;

	/* set up the endpoint information */
	/* use only the first bulk-in and bulk-out endpoints */
	iface_desc = interface->cur_altsetting;
	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		endpoint = &iface_desc->endpoint[i].desc;

		//if((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_CONTROL) {
		printk(KERN_DEBUG "[HALEEQ DEVICE CLASS MODE] usb_endpoint_type(): %d endpoint #: %d\n", usb_endpoint_type(endpoint), usb_endpoint_num(endpoint));
		//}
		if((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT) {
			printk(KERN_DEBUG "[HALEEQ PIANO]: Found [USB_ENDPOINT_XFER_INT] address: %d\n", endpoint->bEndpointAddress);
			buffer_size = endpoint->wMaxPacketSize;
			dev->int_in_size = buffer_size;
			dev->int_in_endpointAddr = endpoint->bEndpointAddress;
			dev->int_in_endpointInterval = endpoint->bInterval;
			dev->int_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->int_in_buffer) {
				printk(KERN_DEBUG "[HALEEQ PIANO ERROR]: Could not allocate int_in_buffer");
				goto error;
			}
			dev->int_in_urb = usb_alloc_urb(0, GFP_KERNEL);
			if(!dev->int_in_urb) {
				printk(KERN_DEBUG "[HALEEQ PIANO ERROR]: Could not allocate int_in_urb");
				goto error;
			}
			
			printk(KERN_DEBUG "[HALEEQ PIANO INTERVAL]: %d\n", dev->int_in_endpointInterval);
			
			usb_fill_int_urb(dev->int_in_urb, dev->udev,
							 usb_rcvintpipe(dev->udev, dev->int_in_endpointAddr),
							 dev->int_in_buffer, 
			                 dev->int_in_size,
			                 interrupt_handler, 
			                 dev,
			                 dev->int_in_endpointInterval);
			dev->int_in_urb->transfer_flags |= URB_NO_TRANSFER_DMA_MAP;
			/*retval = usb_submit_urb(dev->int_in_urb, GFP_KERNEL);
			if (retval != 0) {
				printk(KERN_DEBUG "[HALEEQ PIANO ERROR]: usb_submit_urb error %d \n", retval);
				goto error;
			}*/
			
		}//int_in_endpointAddr

		if (!dev->bulk_in_endpointAddr &&
		    (endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
					== USB_ENDPOINT_XFER_BULK)) {
			/* we found a bulk in endpoint */
			printk(KERN_DEBUG "[HALEEQ PIANO]: Found [USB_ENDPOINT_XFER_BULK IN] address: %d endpoint #: %d\n", endpoint->bEndpointAddress, usb_endpoint_num(endpoint));
			buffer_size = endpoint->wMaxPacketSize;
			dev->bulk_in_size = buffer_size;
			dev->bulk_in_endpointAddr = endpoint->bEndpointAddress;
			dev->bulk_in_buffer = kmalloc(buffer_size, GFP_KERNEL);
			if (!dev->bulk_in_buffer) {
				err("Could not allocate bulk_in_buffer");
				goto error;
			}
		}

		if (!dev->bulk_out_endpointAddr &&
		    !(endpoint->bEndpointAddress & USB_DIR_IN) &&
		    ((endpoint->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK)
					== USB_ENDPOINT_XFER_BULK)) {
			/* we found a bulk out endpoint */
			printk(KERN_DEBUG "[HALEEQ PIANO]: Found [USB_ENDPOINT_XFER_BULK OUT] address: %d endpoint #: %d\n", endpoint->bEndpointAddress, usb_endpoint_num(endpoint));
			dev->bulk_out_endpointAddr = endpoint->bEndpointAddress;
		}
	}
	if (!(/*dev->bulk_in_endpointAddr &&*/ dev->bulk_out_endpointAddr)) {
		printk(KERN_DEBUG "[HALEEQ PIANO]: Could not find both bulk-in and bulk-out endpoints");
		goto error;
	}

	/* save our data pointer in this interface device */
	usb_set_intfdata(interface, dev);

	/* we can register the device now, as it is ready */
	retval = usb_register_dev(interface, &skel_class);
	if (retval) {
		/* something prevented us from registering this driver */
		printk(KERN_DEBUG "[HALEEQ PIANO]: Not able to get a minor for this device.");
		usb_set_intfdata(interface, NULL);
		goto error;
	}

	/* let the user know what node this device is now attached to */
	//dev_info("USB Skeleton device now attached to USBSkel-%d", interface->minor);
	printk(KERN_DEBUG "[HALEEQ]: USB Skeleton device now attached to USBSkel-%d\n", interface->minor);
	
	/*dev->irqN = 9;
	int result = request_irq(dev->irqN, short_interrupt, IRQF_SHARED, "skel0", dev->udev);
	if (result) {
		printk(KERN_INFO "[HALEEQ IRQ]: can't assign irq %i\n", result);
	}
	else { // actually enable it -- assume this *is* a parallel port
		enable_irq(dev->irqN);
		printk(KERN_INFO "[HALEEQ IRQ]: Sucessfully assigned irq %i\n", result);
	}*/
	
	return 0;

error:
	if (dev)
		kref_put(&dev->kref, skel_delete);
	return retval;
}

static void skel_disconnect(struct usb_interface *interface)
{
	struct usb_skel *dev;
	int minor = interface->minor;

	/* prevent skel_open() from racing skel_disconnect() */
	//lock_kernel();

	dev = usb_get_intfdata(interface);
	disable_irq(dev->irqN);
	free_irq(dev->irqN, dev->udev);
	usb_set_intfdata(interface, NULL);

	/* give back our minor */
	usb_deregister_dev(interface, &skel_class);

	//unlock_kernel();

	/* decrement our usage count */
	kref_put(&dev->kref, skel_delete);

	printk(KERN_DEBUG "[HALEEQ PIANO]: USB Skeleton #%d now disconnected\n", minor);
}

static struct usb_driver skel_driver = {
	//.owner = THIS_MODULE,
	.name = "skeleton",
	.id_table = skel_table,
	.probe = skel_probe,
	.disconnect = skel_disconnect,
};

static int __init usb_skel_init(void)
{
	int result;

	/* register this driver with the USB subsystem */
	result = usb_register(&skel_driver);
	if (result)
		printk(KERN_DEBUG "[HALEEQ PIANO DEVICE CLASS MODE]: usb_register failed. Error number %d", result);
	printk(KERN_DEBUG "[HALEEQ PIANO DEVICE CLASS MODE]----> LOADING... USB Skeleton device...\n");

	return result;
}

static void __exit usb_skel_exit(void)
{
	/* deregister this driver with the USB subsystem */
	usb_deregister(&skel_driver);
	printk(KERN_DEBUG "[HALEEQ PIANO]----> UNLOADING... USB Skeleton device...\n");
}

module_init (usb_skel_init);
module_exit (usb_skel_exit);

MODULE_LICENSE("GPL");
