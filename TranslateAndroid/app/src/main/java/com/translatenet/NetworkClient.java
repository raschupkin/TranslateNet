package com.translatenet;

import android.app.Notification;
import android.app.PendingIntent;
import android.media.MediaPlayer;
import android.content.Context;

import java.lang.Runnable;
import java.io.DataOutputStream;
import java.io.InputStream;
import java.net.InetAddress;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.io.*;
import java.security.cert.*;
import java.security.KeyStore;
import java.util.ArrayList;
import java.util.Random;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

import javax.net.ssl.*;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import javax.net.ssl.TrustManagerFactory;

import android.os.Message;
import android.app.Service;
import android.os.Bundle;
import android.os.Handler;
import android.os.Messenger;
import android.content.Intent;
import android.os.IBinder;
import android.telephony.TelephonyManager;
import android.text.format.Time;

public class NetworkClient extends Service {
    public static final String ServerAddress = "translate-net.com";
    //    public static final String ServerAddress = "194.58.108.201";
//	public static final String ServerAddress = "192.168.43.148";
    public static final int ServerPort = 45914;
    public static final String Cert_ServerName = "translate-net.com";
    public static final int PACKETBUF_SIZE = 1048576;
    public static final int TIMEOUT_RECONNECT = 30000;
    public static String DeviceID;

    private static Intent mInstance = null;
    public static final int ACTION_REG = 1;
    public static final int ACTION_UNREG = 2;
    public static final int ACTION_REPLY = 3;
    public static final String PARAM_CON = "CONNECTED";
    public static final int ACTION_CONLOGIN = 4;
    public static final int ACTION_CONREGISTER = 5;
    public static final int ACTION_CONRESET = 6;
    public static final int ACTION_DISCON = 7;
    public static final int ACTION_CON = 8;
    public static final int ACTION_SEND = 9;
    public static final int ACTION_SEND_SET_COUNTRY = 10;

    public static final String PARAM_MSG = "MSG";
    public static final String PARAM_ERR = "ERR";
    public static final String PARAM_EMAIL = "EMAIL";
    public static final String PARAM_PASS = "PASS";
    public static final String PARAM_IST = "IST";

    public static boolean IS_LOGGEDIN = false;

    Boolean NeedConLogin = false;
    Boolean NeedConRegister = false;
    Boolean NeedConReset = false;
    String Email, Pass;
    Boolean isT = null;

    ArrayList<Messenger> mClients = new ArrayList<Messenger>();
    SSLSocket mSocket = null;
    Lock sendLock = new ReentrantLock();
    Thread Thread_ConRecv = null;

    public static final int STATISTIC_TIMEOUT = 300000;
    Time lastStatPacket;

    void sendUser_Message(String type, String msg) {
        try {
            Bundle b = new Bundle();
            b.putString(type, msg);
            for (int i = 0; i < mClients.size(); i++) {
                Message reply = Message.obtain(null, ACTION_REPLY);
                reply.setData(b);
                mClients.get(i).send(reply);
            }
        } catch (Exception ex) {
        }
    }

    void sendUser_Connect(boolean connected, String msg) {
        Bundle b = new Bundle();
        b.putBoolean(PARAM_CON, connected);
        b.putString(PARAM_ERR, msg);
        try {
            for (int i = 0; i < mClients.size(); i++) {
                Message reply = Message.obtain(null, ACTION_CON);
                reply.setData(b);
                mClients.get(i).send(reply);
            }
        } catch (Exception ex) {
        }
    }

    public static final int SERVICE_FGD_ID = 10;

    private void makeForeground() {
        Notification note = new Notification(R.drawable.flag_tj,
                getString(R.string.app_name),
                System.currentTimeMillis());
        Intent i = new Intent(this, LoginActivity.class);
        i.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP |
                Intent.FLAG_ACTIVITY_SINGLE_TOP);
        PendingIntent pi = PendingIntent.getActivity(this, 0,
                i, 0);
        note.setLatestEventInfo(this, getString(R.string.app_name),
                getString(R.string.app_notify),
                pi);
        note.flags |= Notification.FLAG_NO_CLEAR;
        startForeground(SERVICE_FGD_ID, note);
    }

    private void makeBackground() {
        stopForeground(true);
    }

    Parser parser;

    private void PreprocessPacket(String packet) {
        String type = parser.parseType(packet);
        if (type.equalsIgnoreCase("statistic")) {
            lastStatPacket = new Time();
            lastStatPacket.setToNow();
        }
        if (type.equalsIgnoreCase("error")) {
            Parser.PacketError p = parser.parseError(packet);
            if (p == null)
                return;
            if (p.command.equalsIgnoreCase("login")) {
                if (p.code == Parser.ERROR_NOERROR || p.code == Parser.ERROR_NO_PHONE ||
                        p.code == Parser.ERROR_ALREADY_LOGIN || p.code == Parser.ERROR_PHONE_CHANGED) {
                    IS_LOGGEDIN = true;
                    if (Country.isCountry(MainActivity.curCountry))
                        sendPacket_SetCountry(MainActivity.curCountry);
                } else if (p.code == Parser.ERROR_WRONG_PASSWORD) {
                    IS_LOGGEDIN = false;
                    Disconnect();
                }
            } else if (p.command.equalsIgnoreCase("set_country")) {
                if (p.code != Parser.ERROR_NOERROR)
                     if (isT == null || isT) {
                        IS_LOGGEDIN = false;
                        Disconnect();
                    }
            } else if (p.command.equalsIgnoreCase("reset_password") ||
                     p.command.equalsIgnoreCase("register_user")) {
                IS_LOGGEDIN = false;
                Disconnect();
            }
        } else if (type.equalsIgnoreCase("user_data")) {
            Options options = new Options();
            User user = parser.parseUserData(packet, options);
            if (user == null)
                return;
            isT = user.isT;
            stopStatisticThread();
            startStatisticThread();

            if (user.isT)
                makeForeground();
            else
                makeBackground();
        }
        if (type.equalsIgnoreCase("phonecall_request")) {
            Parser.PacketPhonecallRequest p = parser.parsePhonecallRequest(packet);
            if (p == null)
                return;
            MediaPlayer mPlayer = MediaPlayer.create(getApplicationContext(), R.raw.call_request);
            mPlayer.start();
        } else if (type.equalsIgnoreCase("phonecall_confirm")) {
            Parser.PacketPhonecallConfirm p = parser.parsePhonecallConfirm(packet);
            if (p == null)
                return;
            MediaPlayer mPlayer;
            if (p.accept)
                mPlayer = MediaPlayer.create(getApplicationContext(), R.raw.call_confirm);
            else
                mPlayer = MediaPlayer.create(getApplicationContext(), R.raw.call_reject);
            mPlayer.start();
        }
    }

    Thread statisticThread;
    public static final int TIMER_STATISTIC = 30000;

    protected void startStatisticThread() {
        statisticThread = new Thread(new Runnable() {
            public void run() {
                while (true) {
                    try {
                        if (Thread.currentThread().isInterrupted())
                            return;
                        sendPacket_GetStatistic();
                        synchronized (this) {
                            wait(TIMER_STATISTIC);
                        }
                    } catch (InterruptedException ex) {
                        return;
                    } catch (Exception ex) {
                    }
                }
            }
        });
        statisticThread.start();
    }

    protected void stopStatisticThread() {
        if (statisticThread != null)
            statisticThread.interrupt();
        statisticThread = null;
    }

    boolean Connected = false;

    synchronized void onConnected(boolean connected, String msg) {
        if (!connected) {
            IS_LOGGEDIN = false;
            if (mSocket != null)
                try {
                    mSocket.close();
                } catch (Exception ex) {
                } finally {
                    mSocket = null;
                }
        }
        if (connected != Connected) {
            sendUser_Connect(connected, msg);
        }
        Connected = connected;

        if (connected) {
            Thread sendThread = new Thread(new Runnable() {
                @Override
                public void run() {
                    sendLock.lock();
                    try {
                        if (NeedConLogin)
                            SendPacket_Login(Email, Pass);
                        else if (NeedConRegister) {
                            NeedConRegister = false;
                            SendPacket_Register(Email, isT);
                        } else if (NeedConReset) {
                            NeedConReset = false;
                            SendPacket_ResetPassword(Email);
                        }
                    } catch (Exception ex) {
                        sendUser_Message(PARAM_ERR, ex.getMessage());
                    } finally {
                        sendLock.unlock();
                    }
                }
            });
            sendThread.start();
        }
    }

    private void ConRecvThread() {
//        makeForeground();
        boolean recon_sent = false;
        while (true) {
            if (Thread.interrupted())
                return;
            try {
                if ((mSocket = EstablishConnection()) != null) {
                    mSocket.setTcpNoDelay(true);
                    break;
                }
//                mSocket.setSoTimeout(1000);
//				SystemClock.sleep(DELAY_RECON);
            } catch (Exception ex) {
                if (!recon_sent) {
                    onConnected(false, ex.getMessage());
                    recon_sent = true;
//					return;
                }
                //SystemClock.sleep(DELAY_RECON);
                return;
            }
            return;
        }

        onConnected(true, "");
        lastStatPacket = new Time();
        lastStatPacket.setToNow();
        recon_sent = false;

        while (!Thread.interrupted()) {
            try {
                String packet = recvPacket();
                if (packet.length() > 0) {
                    PreprocessPacket(packet);
                    sendUser_Message(PARAM_MSG, packet);
                } else {
                    onConnected(false, getString(R.string.network_error_connection)); // sent from recvPacket
                    break;
                }
            }/* catch (SocketException ex) {
				break;
			}*/ catch (Exception ex) {
//					sendUser_Connect(false, getString(R.string.network_error_connection));
                onConnected(false, ex.getMessage());
                break;
            }
        }
        if (mSocket != null) {
            try {
                mSocket.close();
            } catch (Exception ex) {
            } finally {
                mSocket = null;
            }
        }
    }

    private Thread startConRecvThread() {
        mSocket = null;
        Thread thread = new Thread(new Runnable() {
            public void run() {
                ConRecvThread();
            }
        });
        thread.start();
        return thread;
    }

    public void Disconnect() {
        if (Thread_ConRecv != null) {
            Thread_ConRecv.interrupt();
            Thread_ConRecv = null;
        }
        if (mSocket != null) {
            try {
                mSocket.close();
                mSocket = null;
            } catch (Exception ex) {
            }
        }
    }

    void TryConnect() {
        Disconnect();
        sendLock = new ReentrantLock();
        if (Thread_ConRecv == null)
            Thread_ConRecv = startConRecvThread();
    }

    void sendPacket(final String packet) {
        if (mSocket == null) {
            onConnected(false, "");
            return;
        }
        Thread sendThread = new Thread(new Runnable() {
            @Override
            public void run() {
                sendLock.lock();
                try {
                    do_sendPacket(packet);
                } catch (Exception ex) {
//                            sendUser_Message(PARAM_ERR, ex.getMessage());
                    onConnected(false, ex.getMessage());
                } finally {
                    sendLock.unlock();
                }
            }
        });
        sendThread.start();
    }

    protected void sendPacket_SetCountry(String country){
        if (country == null)
            return;
        String packet = "<set_country>";
        packet += "<country>";
        packet += country;
        packet += "</country>";
        packet += "</set_country>";
        sendPacket(packet);
    }

    class IncomingHandler extends Handler {
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case ACTION_REG:
                    if (!mClients.contains(msg.replyTo))
                        mClients.add(msg.replyTo);
                    break;
                case ACTION_UNREG:
                    if (mClients.contains(msg.replyTo))
                        mClients.remove(msg.replyTo);
                    break;
                case ACTION_CONLOGIN:
                    Email = msg.getData().getString(PARAM_EMAIL);
                    Pass = msg.getData().getString(PARAM_PASS);
                    NeedConLogin = true;
                    NeedConRegister = false;
                    NeedConReset = false;
                    DetermineDeviceID();
                    TryConnect();
                    break;
                case ACTION_SEND_SET_COUNTRY:
                    sendPacket_SetCountry(MainActivity.curCountry);
                    break;
                case ACTION_CONREGISTER:
                    Email = msg.getData().getString(PARAM_EMAIL);
                    isT = msg.getData().getBoolean(PARAM_IST);
                    NeedConLogin = false;
                    NeedConRegister = true;
                    NeedConReset = false;
                    TryConnect();
                    break;
                case ACTION_CONRESET:
                    Email = msg.getData().getString(PARAM_EMAIL);
                    NeedConLogin = false;
                    NeedConRegister = false;
                    NeedConReset = true;
                    TryConnect();
                    break;
                case ACTION_DISCON:
                    Disconnect();
                    makeBackground();
                    break;
                case ACTION_SEND: {
                    final String packet = msg.getData().getString(PARAM_MSG);
                    sendPacket(packet);
                    break;
                }
                default:
                    super.handleMessage(msg);
            }
        }
    }

    final Messenger mMessenger = new Messenger(new IncomingHandler());

    @Override
    public IBinder onBind(Intent intent) {
        return mMessenger.getBinder();
    }

    public int sendPacket_GetStatistic() throws Exception {
        return do_sendPacket("<get_statistic></get_statistic>");
    }

    void DetermineDeviceID() {
        TelephonyManager tm = (TelephonyManager) getBaseContext().getSystemService(Context.TELEPHONY_SERVICE);
        DeviceID = tm.getDeviceId() + "_" + tm.getSimSerialNumber();
    }

    protected int SendPacket_Login(String email, String password) throws Exception {
//        String deviceID = getDeviceID();
        if (email == null || password == null || DeviceID == null)
            return -1;
        return do_sendPacket("<login><os>android</os>" +
                "<version>" + LoginActivity.Version + "</version>" +
                "<email>" + email + "</email>" +
                "<password>" + password + "</password>" +
                "<device_id>" + DeviceID + "</device_id></login>");
    }
    protected int SendPacket_Register(String email, Boolean isT) throws Exception {
        if (email == null)
            return -1;
        return do_sendPacket("<register_user>" +
                "<email>" + email + "</email>" +
                "<is_translator>" + (isT ? "1" : "0") + "</is_translator>" +
                "</register_user>");
    }
    public int SendPacket_ResetPassword(String email) throws Exception {
        if (email == null)
            return -1;
        String packet = "<reset_password>";
        packet += "<email>";	packet += email;	packet += "</email>";
        packet += "</reset_password>";
        return do_sendPacket(packet);
    }

    // must be synchronized on sendLock
    protected int do_sendPacket(String msg) throws Exception {
        if (mSocket == null || msg == null) {
      //      onConnected(false, getString(R.string.network_error_connection));
            return -1;
            //throw new RuntimeException();
        }
        if (!mSocket.isConnected()) {
//            onConnected(false, getString(R.string.network_error_connection));
            return -1;
        }
        if (4 + msg.length() > PACKETBUF_SIZE) {
      //      onConnected(false, getString(R.string.network_error_connection));
            return -1;
        }
        DataOutputStream os = new DataOutputStream(mSocket.getOutputStream());
        ByteBuffer bb = ByteBuffer.allocateDirect(4).order(ByteOrder.LITTLE_ENDIAN);
        bb.putInt(msg.length());
        bb.position(0);
        byte[] hdr = new byte[bb.remaining()];
        bb.get(hdr);
        os.write(hdr);
        os.write(msg.getBytes());
//        os.flush();
        os.close();
        return 0;
	}
    static final int HEADER_LEN = 4;
	protected String recvPacket() throws Exception {
		if (mSocket == null) {
//            onConnected(false, getString(R.string.network_error_connection));
            throw new RuntimeException();
        }
        if (!mSocket.isConnected()) {
//            onConnected(false, getString(R.string.network_error_connection));
            return "";
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        DataInputStream is = new DataInputStream(mSocket.getInputStream());
        byte[] hdr = new byte[HEADER_LEN];
        int count = is.read(hdr);
        if (count < HEADER_LEN) {
//            onConnected(false, getString(R.string.network_error_connection));
            return "";
        }
        ByteBuffer bb = ByteBuffer.wrap(hdr);
        bb.order(ByteOrder.LITTLE_ENDIAN);
        int len = bb.getInt(0);
        if (len > PACKETBUF_SIZE) {
//            onConnected(false, getString(R.string.network_error_connection));
            return "";
        }
        byte[] buf = new byte[len];
        int pos = 0;
        while (pos < len) {
            count = is.read(buf);
            if (count < 0) {
//                onConnected(false, getString(R.string.network_error_connection));
                return "";
            }
            out.write(buf, 0, count);
            pos += count;
        }
		return out.toString();
	}

	SSLSocket EstablishConnection()  {
		try {
			CertificateFactory cf = CertificateFactory.getInstance("X.509");
			// From https://www.washington.edu/itconnect/security/ca/load-der.crt
	//		InputStream caInput = new BufferedInputStream(new FileInputStream("/data/data/com.translatenet/files/translate_cert.pem"));
			//String CertPath = getFilesDir() + "/" + MainActivity.File_Cert;
			InputStream caInput = getResources().getAssets().open(MainActivity.File_CACert);
			Certificate ca;
			try {
			    ca = cf.generateCertificate(caInput);
			    System.out.println("ca=" + ((X509Certificate) ca).getSubjectDN());
			} finally {
			    caInput.close();
			}
	
			// Create a KeyStore containing our trusted CAs
			String keyStoreType = KeyStore.getDefaultType();
			KeyStore keyStore = KeyStore.getInstance(keyStoreType);
			keyStore.load(null, null);
	//		keyStore.setCertificateEntry("ca", ca);
	
			// Create a TrustManager that trusts the CAs in our KeyStore
			String tmfAlgorithm = TrustManagerFactory.getDefaultAlgorithm();
			TrustManagerFactory tmf = TrustManagerFactory.getInstance(tmfAlgorithm);
//			tmf.init(keyStore);

			// Create an SSLContext that uses our TrustManager
			SSLContext context = SSLContext.getInstance("TLS");
	//		context.init(null, tmf.getTrustManagers(), null);
			context.init(null, null, null);
			SSLSocketFactory sf = context.getSocketFactory();
//			SSLSocket mSocket = (SSLSocket)sf.createSocket(ServerAddress, ServerPort);
            InetAddress addr = InetAddress.getByName(ServerAddress);
            SSLSocket mSocket = (SSLSocket)sf.createSocket(addr, ServerPort);
			HostnameVerifier hv = HttpsURLConnection.getDefaultHostnameVerifier();
			SSLSession session = mSocket.getSession();
	
			// Verify certicate hostname
			// This is due to lack of SNI support in the current SSLSocket.
			if (!hv.verify(Cert_ServerName, session)) {
			    throw new SSLHandshakeException("Expected certificate " + Cert_ServerName + ", found " + session.getPeerPrincipal());
			}
			mSocket.startHandshake();
			return mSocket;
		
		} catch (Exception ex) {
            sendUser_Message(PARAM_ERR, getString(R.string.network_error_connect));
			return null;
		}
	}

	public static Intent getInstance(Context context) {
		if (mInstance == null) {
			mInstance = new Intent(context, NetworkClient.class);
		}
		return mInstance;
	}
    Timer reconnectTimer = null;
    protected void createReconnectTimer() {
        reconnectTimer = new Timer();
        Random rand = new Random();
        int delay = TIMEOUT_RECONNECT/2;
        delay += rand.nextInt(TIMEOUT_RECONNECT/2);
        reconnectTimer.schedule(new TimerTask() {
            public void run() {
                if (!IS_LOGGEDIN) {
                    if (MainActivity.WAS_LOGGEDIN) {
                        TryConnect();
                    } else {
                        makeBackground();
                    }
                }

                if (lastStatPacket != null) {
                    Time now = new Time();
                    now.setToNow();
                    if (lastStatPacket.toMillis(true) + STATISTIC_TIMEOUT < now.toMillis(true)) {
                        try {
                            if (mSocket != null)
                                mSocket.close();
                        } catch (Exception ex) {

                        }
                    }
                }
            }
        }, delay, TIMEOUT_RECONNECT);
    }
    protected void cancelReconnectTimer() {
        if (reconnectTimer != null)
            reconnectTimer.cancel();
        reconnectTimer = null;
    }
    @Override
    public void onCreate() {
        super.onCreate();
        parser = new Parser();
        createReconnectTimer();
    }

    @Override
    public void onDestroy() {
        stopStatisticThread();
        cancelReconnectTimer();
        super.onDestroy();
    }

}
