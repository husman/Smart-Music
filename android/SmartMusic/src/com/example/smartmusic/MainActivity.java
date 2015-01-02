package com.example.smartmusic;

import java.util.HashMap;
import java.util.Iterator;

import com.smartmusic.udpconnection.Client;

import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;
import android.os.Bundle;
import android.os.Handler;
import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.TextView;
import android.widget.ToggleButton;

public class MainActivity extends Activity {
	
	private byte[] bytes;
	private static int TIMEOUT = 0;
	private boolean forceClaim = true;
	private UsbManager mUsbManager;
	private static final String TAG = "MainActivity";
	private PendingIntent mPermissionIntent;
	private TextView txt1;
	private int i = 0;
	
	private UsbDevice device;
	private UsbDeviceConnection connection1, connection2;
	private UsbEndpoint in_endpoint1, in_endpoint2;
	private UsbEndpoint out_endpoint1, out_endpoint2;
	
	private Context mContext;
	
	private static final String ACTION_USB_PERMISSION = "com.android.example.USB_PERMISSION";
	
	private void redirectDeviceData() {
		Handler hdl = new Handler();
		hdl.post(new Runnable() {
			
			@Override
			public void run() {
				while(true) {
					byte[] in_bytes = new byte[4];
					
					connection2.bulkTransfer(in_endpoint2, in_bytes, in_bytes.length,TIMEOUT);
					connection1.bulkTransfer(out_endpoint1, in_bytes, in_bytes.length, TIMEOUT);
					
					txt1.append("[TRANSFERED]: " 
		            		+ String.format("%02x",in_bytes[0]) + String.format(" %02x",in_bytes[1])
		            		+ String.format(" %02x",in_bytes[2]) + String.format(" %02x",in_bytes[3])
		            		+ "\n");
				}
			}
		});
	}
	
	private void communicateWithDevice() {
		txt1.append("Permission granted [0]\n");
		UsbInterface intf;
		
		for(int z=0; z<device.getInterfaceCount(); ++z) {
			intf = device.getInterface(z);
			txt1.append("Device product id: " +device.getProductId()+"\n");
			txt1.append("Number of interfaces: " +device.getInterfaceCount()+"\n");
			txt1.append("Number of endpoints: " +intf.getEndpointCount()+"\n");
			
			for(int i=0; i<intf.getEndpointCount(); ++i)
				if(intf.getEndpoint(i).getDirection() == UsbConstants.USB_DIR_OUT) {
					if(device.getVendorId() == 1999 && device.getProductId() == 26626) {
						out_endpoint1 = intf.getEndpoint(i);
						txt1.append("out_endpoint1 ("+i+"): USB_DIR_OUT\n");
					} else if(device.getVendorId() == 1999 && device.getProductId() == 26627) {
						out_endpoint2 = intf.getEndpoint(i);
						txt1.append("out_endpoint2 ("+i+"): USB_DIR_OUT\n");
					}
				} else {
					if(device.getVendorId() == 1999 && device.getProductId() == 26626) {
						in_endpoint1 = intf.getEndpoint(i);
						txt1.append("in_endpoint1 ("+i+"): USB_DIR_OUT\n");
					} else if(device.getVendorId() == 1999 && device.getProductId() == 26627) {
						in_endpoint2 = intf.getEndpoint(i);
						txt1.append("in_endpoint2 ("+i+"): USB_DIR_OUT\n");
					}
				}
			
			if(intf.getEndpointCount() > 0) {
				if(device.getVendorId() == 1999 && device.getProductId() == 26626) {
					connection1 = mUsbManager.openDevice(device); 
					connection1.claimInterface(intf, forceClaim);
				} else if(device.getVendorId() == 1999 && device.getProductId() == 26627) {
					connection2 = mUsbManager.openDevice(device); 
					connection2.claimInterface(intf, forceClaim);	
					//redirectDeviceData();
				}
			}
		}
		
		//new Thread(new Client(txt1,connection,endpoint)).start();
		
		/*bytes = new byte[4];
		bytes[0] = (byte)0x09;
		bytes[1] = (byte)0x90;
		bytes[2] = (byte)0x3c;
		bytes[3] = (byte)0x40;
		connection.bulkTransfer(endpoint, bytes, bytes.length, TIMEOUT);*/
   	    
	}
	
private final BroadcastReceiver mUsbReceiver = new BroadcastReceiver() {

    public void onReceive(Context context, Intent intent) {
        String action = intent.getAction();
        if (ACTION_USB_PERMISSION.equals(action)) {
            synchronized (this) {
            	device = (UsbDevice)intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);

                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    if(device != null){
                      //call method to set up device communication
                    	communicateWithDevice();
                   }
                } 
                else {
                	txt1.append("Permission not granted to [-]\n");
                }
            }
        }
    }
};

private OnClickListener btn1Handler = new OnClickListener() {
	@Override
	public void onClick(View v) {
		txt1.append("Button 1 clicked setting up [0]\n");
		HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
		Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
		UsbDevice tmp_device;
		i = 0;
		while(deviceIterator.hasNext()){
			tmp_device = deviceIterator.next();
		    //your code
		    if(tmp_device != null) {
				mPermissionIntent = PendingIntent.getBroadcast(mContext, 0, new Intent(ACTION_USB_PERMISSION), 0);
				IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
				if(tmp_device.getVendorId() == 1999 && tmp_device.getProductId() == 26626) {
					txt1.append("\n\n["+i+"] ==> \n\n"+tmp_device+"\n");
					registerReceiver(mUsbReceiver, filter);
					mUsbManager.requestPermission(tmp_device, mPermissionIntent);
					break;
				}
				++i;
			}
		}
	}
};

private OnClickListener btn2Handler = new OnClickListener() {
	@Override
	public void onClick(View v) {
		txt1.append("Button 2 clicked setting up [1]\n");
		HashMap<String, UsbDevice> deviceList = mUsbManager.getDeviceList();
		Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
		UsbDevice tmp_device;
		i = 0;
		while(deviceIterator.hasNext()){
			tmp_device = deviceIterator.next();
		    //your code
		    if(tmp_device != null) {
				mPermissionIntent = PendingIntent.getBroadcast(mContext, 0, new Intent(ACTION_USB_PERMISSION), 0);
				IntentFilter filter = new IntentFilter(ACTION_USB_PERMISSION);
				if(tmp_device.getVendorId() == 1999 && tmp_device.getProductId() == 26627) {
					txt1.append("\n\n["+i+"] ==> \n\n"+tmp_device+"\n");
					registerReceiver(mUsbReceiver, filter);
					mUsbManager.requestPermission(tmp_device, mPermissionIntent);
					break;
				}
				++i;
			}
		}
	}
};

private OnClickListener btn3Handler = new OnClickListener() {
	@Override
	public void onClick(View v) {
		txt1.append("Button 3 clicked setting up [NETWORK]\n");
		new Thread(new Client(txt1, connection2, out_endpoint2)).start();
	}
};

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		txt1 = (TextView) findViewById(R.id.txt1);
		txt1.setText("");
		
		mContext = this;
		
		mUsbManager = (UsbManager) getSystemService(Context.USB_SERVICE);
		
		Button btn1 = (Button)findViewById(R.id.button1);
		Button btn2 = (Button)findViewById(R.id.button2);
		Button btn3 = (Button)findViewById(R.id.button3);
		
		btn1.setOnClickListener(btn1Handler);
		btn2.setOnClickListener(btn2Handler);
		btn3.setOnClickListener(btn3Handler);
		
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

}
