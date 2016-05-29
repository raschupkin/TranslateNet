package com.translatenet;

import android.content.SharedPreferences;

import android.support.v4.app.FragmentTransaction;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.app.AlertDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.Messenger;
import android.support.v7.app.ActionBarActivity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.TextView;
import android.widget.Toast;

/**
 * Activity which displays a login screen to the user, offering registration as
 * well.
 */
public class LoginActivity extends ActionBarActivity {
	public static final int Version = 0;
	boolean Connected;
	LoginFragment loginFragment;
	RegisterFragment registerFragment;
	ResetFragment resetFragment;
	boolean ActivityStopped = false;
	public boolean isActivityStopped() {
		return ActivityStopped;
	}
	final int MAIN_REQUESTCODE = 1;
	void StartMainActivity() {
		//startActivity(new Intent(LoginActivity.this, MainActivity.class));
		Intent intent = new Intent("android.intent.action.LIST");
	    startActivityForResult(intent, MAIN_REQUESTCODE);
	}
	protected void onActivityResult (int requestCode, int resultCode, Intent data) {
		if (requestCode == MAIN_REQUESTCODE) {
            Disconnect();
            if (MainActivity.curCountry != null && !Country.isCountry(MainActivity.curCountry))
                UserMessage(getString(R.string.error), MainActivity.getErrorMessage(getApplicationContext(), Parser.ERROR_UNKNOWN_COUNTRY));
//			Connect(false);
		}
	}
	
	public int sendPacket_ResetPassword(String email) {
		String packet = "<reset_password>";
		packet += "<email>";	packet += email;	packet += "</email>";
		packet += "</reset_password>";
		return sendPacket(packet);
	}
	public int sendPacket_RegisterUser(String email, boolean isT) {
		String packet = "<register_user>";
		packet += "<email>";	packet += email;	packet += "</email>";
		packet += "<is_translator>";	packet += isT ? "1" : "0";	packet += "</is_translator>";
		packet += "</register_user>";
		return sendPacket(packet);
	}
	public int sendPacket_GetLanguages() {
		return sendPacket("<get_languages></get_languages>");
	}
	public int sendPacket_SetBusy(boolean busy) {
		String packet = "<set_busy>";
		packet += "<busy>";	packet += busy ? 1 : 0;	packet += "</busy>";
		packet += "</set_busy>";
		return sendPacket(packet);
	}
	public int sendPacket(String packet) {
		Message msg = Message.obtain(null, NetworkClient.ACTION_SEND, 0, 0);
		Bundle b = new Bundle();
		b.putString(NetworkClient.PARAM_MSG, packet);
		msg.setData(b);
        try {
        	mService.send(msg);
        } catch (Exception ex) {
        	UserMessage(getString(R.string.error), "System error.");
        	Log.e("Error", "[" + ex.toString() + "]:"  + ex.getMessage());
        }
		return 0;
	}

	Parser parser;
	protected int onPacketReceived(String packet) {
		String type = parser.parseType(packet);
		if (type == null) {
			UserMessage(getString(R.string.error_network), getString(R.string.network_error_format));
			showProgress(false, PROGRESS_LOAD);
			return -1;
		}
		if (type.compareToIgnoreCase("error") == 0) {
			Parser.PacketError p = parser.parseError(packet);
			if (p == null) {
				UserMessage(getString(R.string.error), MainActivity.getErrorMessage(getApplicationContext(), Parser.ERROR_OTHER));
				showProgress(false, PROGRESS_LOAD);
				return -1;
			}
			if (p.code == Parser.ERROR_VERSION) {
				showProgress(false, PROGRESS_LOAD);
                String msg = "";
                if (p.message != null && p.message.length()>0)
                    msg = p.message;
				UserMessage(MainActivity.getErrorMessage(getApplicationContext(), p.code), msg);
				return -1;
			}
			if (p.command.compareToIgnoreCase("login") == 0) {
                if (loginFragment != null) {
                    loginFragment.onPacket_LoginError(p.code);
//            showProgress(true, PROGRESS_LOAD);
                }
			} else if (p.command.compareToIgnoreCase("register_user") == 0) {
				if (registerFragment != null) {
					registerFragment.onPacket_Error(p.code);	
					showProgress(false, PROGRESS_LOAD);
                    Disconnect();
				}
			} else if (p.command.compareToIgnoreCase("reset_password") == 0) {
				if (resetFragment != null) {
					resetFragment.onPacket_Error(p.code);	
					showProgress(false, PROGRESS_LOAD);
				}				
			} else if (p.command.compareToIgnoreCase("set_busy") == 0) {
            } else
				showProgress(false, PROGRESS_LOAD);
		} else if (type.compareToIgnoreCase("languages") == 0) {
			showProgress(false, PROGRESS_LOAD);
			Lang.Langs.clear();
			if (parser.parseLangsPacket(packet, Lang.Langs) != 0) {
				UserMessage(getString(R.string.error), MainActivity.getErrorMessage(getApplicationContext(), Parser.ERROR_OTHER));
				Disconnect();
				return -1;
			}
/*			try {
				if (parser.parseLDictFile(getResources().openRawResource(R.raw.ldict), Lang.Langs) != 0) {
					throw new Exception();
				}
			} catch (Exception ex) {
				UserMessage(getString(R.string.error), MainActivity.getErrorMessage(getApplicationContext(), Parser.ERROR_OTHER));				
			}
*/		    StartMainActivity();
            return 0;
		} else if (type.compareToIgnoreCase("await_login_confirm") == 0) {
			if (registerFragment != null)
				registerFragment.onPacket_AwaitLoginConfirm();
		}
		return 0;
	}

	boolean mBound;
	Messenger mService;
	boolean ReportedConnectError = false;
	final Messenger mNClientMessenger = new Messenger(new IncomingHandler());
	class IncomingHandler extends Handler {
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case NetworkClient.ACTION_REPLY:
				if (msg.getData().containsKey(NetworkClient.PARAM_MSG)) {
					String packet = msg.getData().getString(NetworkClient.PARAM_MSG);
					onPacketReceived(packet);
				} else if (msg.getData().containsKey(NetworkClient.PARAM_ERR)) {
					if (loginFragment != null)
						loginFragment.onError();
					Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.error_network) + ": " + 
													msg.getData().getString(NetworkClient.PARAM_ERR), Toast.LENGTH_SHORT);
					toast.show();
				//	Disconnect();
				}
				break;
			case NetworkClient.ACTION_CON:
/*				if (!msg.getData().getBoolean(NetworkClient.PARAM_CON) && Connected) {
					Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.network_error_caption) + ": " + 
													msg.getData().getString(NetworkClient.PARAM_ERR), Toast.LENGTH_LONG);
					toast.show();
				}
*/				Connected = msg.getData().getBoolean(NetworkClient.PARAM_CON);
				Connecting = false;
				if (Connected) {
					ReportedConnectError = false;
					Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.network_connected), Toast.LENGTH_SHORT);
					toast.show();
					if (loginFragment != null)
						loginFragment.onConnected();
					if (registerFragment != null)
						registerFragment.onConnected();
					if (resetFragment != null)
						resetFragment.onConnected();

				} else {
					boolean doingLogin = curProgress;
					showProgress(false, PROGRESS_LOAD);
					if (doingLogin && !ReportedConnectError) {
						String err_msg = msg.getData().getString(NetworkClient.PARAM_ERR);
						if (err_msg.length() > 0) {
							Toast toast = Toast.makeText(getApplicationContext(), getString(R.string.network_error_connect) + ": " + 
									err_msg, Toast.LENGTH_SHORT);
							toast.show();
						}
						ReportedConnectError = true;
					}
				}
				break;
			default:
				super.handleMessage(msg);
				break;
			}
		}
	}

    public void ResetConnectError(boolean resetErr) {
        if (resetErr)
            ReportedConnectError = false;
    }

	public boolean isConnected() {
		return Connected;
	}
	boolean Connecting;
	public void ConnectLogin(String email, String pass, String country) {
		Connecting = true;
		Message msg = Message.obtain(null, NetworkClient.ACTION_CONLOGIN, 0, 0);
        Bundle b = new Bundle();
        b.putString(NetworkClient.PARAM_EMAIL, email);
        b.putString(NetworkClient.PARAM_PASS, pass);
        msg.setData(b);
        try {
			mService.send(msg);
		} catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
			Log.e(getString(R.string.error), ex.toString() + ":"  + ex.getMessage());
		}		
	}
    public void ConnectRegister(String email, boolean isT) {
        Connecting = true;
        Message msg = Message.obtain(null, NetworkClient.ACTION_CONREGISTER, 0, 0);
        Bundle b = new Bundle();
        b.putString(NetworkClient.PARAM_EMAIL, email);
        b.putBoolean(NetworkClient.PARAM_IST, isT);
        msg.setData(b);
        try {
            mService.send(msg);
        } catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
            Log.e(getString(R.string.error), ex.toString() + ":"  + ex.getMessage());
        }
    }
    public void ConnectReset(String email) {
        Connecting = true;
        Message msg = Message.obtain(null, NetworkClient.ACTION_CONRESET, 0, 0);
        Bundle b = new Bundle();
        b.putString(NetworkClient.PARAM_EMAIL, email);
        msg.setData(b);
        try {
            mService.send(msg);
        } catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
            Log.e(getString(R.string.error), ex.toString() + ":"  + ex.getMessage());
        }
    }
	public void Disconnect() {
Connected = false;
//		ReportedConnectError = true;
		Message msg = Message.obtain(null, NetworkClient.ACTION_DISCON, 0, 0);
		try {
			mService.send(msg);
		} catch (Exception ex) {
//			UserMessage(getString(R.string.error), "System error.");
			Log.e(getString(R.string.error), ex.toString() + ":"  + ex.getMessage());
		}
	}
	private ServiceConnection mNClientConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            // This is called when the connection with the service has been
            // established, giving us the object we can use to
            // interact with the service.  We are communicating with the
            // service using a Messenger, so here we get a client-side
            // representation of that from the raw IBinder object.
            mService = new Messenger(service);
            mBound = true;
    		Message msg = Message.obtain(null, NetworkClient.ACTION_REG, 0, 0);
    		msg.replyTo = mNClientMessenger;
            try {
            	mService.send(msg);
            } catch (Exception ex) {
            	UserMessage("Error", "System error.");
            	Log.e("Error", ex.toString() + ":"  + ex.getMessage());
            }


            if (NetworkClient.IS_LOGGEDIN) {                // for the case of death in background
                if (loginFragment != null)
                    loginFragment.onPacket_LoginError(Parser.ERROR_NOERROR);
                return;
            }
        }

        public void onServiceDisconnected(ComponentName className) {
            // This is called when the connection with the service has been
            // unexpectedly disconnected -- that is, its process crashed.
            mService = null;
            mBound = false;
        }
    };
    boolean RegisterFragmentOpened = false;
    boolean RegisterFragmentAdded = false;
    public void openRegisterFragment() {
		FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
		if (!RegisterFragmentAdded) {
			ft.add(R.id.frame_login_main, registerFragment);
			RegisterFragmentAdded = true;
		}
		ft.hide(loginFragment);
		ft.show(registerFragment);
		ft.commit();    	
		RegisterFragmentOpened = true;
    }
    public void closeRegisterFragment() {
		FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
		ft.hide(registerFragment);
		ft.show(loginFragment);
		ft.commit();    	    	
		RegisterFragmentOpened = false;
		loginFragment.loadDefaultEmail();
    }
    boolean ResetFragmentOpened = false;
    boolean ResetFragmentAdded = false;
    public void openResetFragment() {
		FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
		if (!ResetFragmentAdded) {
			ft.add(R.id.frame_login_main, resetFragment);
			ResetFragmentAdded = true;
		}
		ft.hide(loginFragment);
		ft.show(resetFragment);
		ft.commit();    	
		ResetFragmentOpened = true;
    }
    public void closeResetFragment() {
		FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
		ft.hide(resetFragment);
		ft.show(loginFragment);
		ft.commit();
		ResetFragmentOpened = false;
    }
	public void onBackPressed() {
		// TODO Auto-generated method stub
		if (RegisterFragmentOpened) {
			closeRegisterFragment();
			return;
		}
        if (ResetFragmentOpened) {
            closeResetFragment();
            return;
        }
        if (Connecting) {
            Disconnect();
        }
		if (curProgress)
			showProgress(false, PROGRESS_LOAD);
		else
			super.onBackPressed();
	}
	protected void onStart() {
		super.onStart();

		ActivityStopped = false;
////		Connected = false;
////		Disconnect();
		if (mBound) {
    		Message msg = Message.obtain(null, NetworkClient.ACTION_REG, 0, 0);
    		msg.replyTo = mNClientMessenger;
            try {
            	mService.send(msg);
            } catch (Exception ex) {
            	UserMessage(getString(R.string.error), "System error.");
            	Log.e("Error", ex.toString() + ":"  + ex.getMessage());
            }
 //   		if (!Connected)
 //   			Connect(false);
		}

        if (NetworkClient.IS_LOGGEDIN) {   // in case screen was locked while NetworkManager reconnected and logged in
            loginFragment.onPacket_LoginError(Parser.ERROR_NOERROR);
            return;
        }
    }
	protected void onStop() {
		super.onStop();
		ActivityStopped = true;
		Message msg = Message.obtain(null, NetworkClient.ACTION_UNREG, 0, 0);
		msg.replyTo = mNClientMessenger; 
		try {
			mService.send(msg);
		} catch (Exception ex) {
			UserMessage(getString(R.string.error), "System error.");
			Log.e("Error", ex.toString() + ":"  + ex.getMessage());
		}
	}
	void UserMessage(String title, String msg) {
		if (isFinishing())
			return;
		AlertDialog.Builder dlgBuilder = new AlertDialog.Builder(this);
		dlgBuilder.setTitle(title);
		dlgBuilder.setMessage(msg);
		dlgBuilder.setCancelable(false);
		dlgBuilder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
				dialog.cancel();
			}
		});
		AlertDialog dlg1 = dlgBuilder.create();
		dlg1.show();	
	}

	@Override
	protected void onResume() {
		// TODO Auto-generated method stub
		super.onResume();
		ActivityStopped = false;
	}
	void CreateFragments() {
	   	loginFragment = new LoginFragment();
    	loginFragment.init(this);
	   	registerFragment = new RegisterFragment();
    	registerFragment.init(this);
	   	resetFragment = new ResetFragment();
    	resetFragment.init(this);
	}
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		setContentView(R.layout.activity_login);
		mFormView = findViewById(R.id.AL_Form);
		mStatusView = findViewById(R.id.AL_status);
		mStatusMessageView = (TextView)findViewById(R.id.AL_status_message);

		CreateFragments();
		FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
		ft.add(R.id.frame_login_main, loginFragment);
		ft.show(loginFragment);
		ft.commit();

        bindService(NetworkClient.getInstance(this), mNClientConnection, Context.BIND_AUTO_CREATE);

		parser = new Parser();
		try {
			parser.parseCountriesFile(getResources().getAssets().open(MainActivity.File_Countries), Country.Countries);
		} catch (Exception ex) {
			UserMessage(getString(R.string.error), ex.getMessage());
//			Close();
		}
	}

	@Override
	protected void onDestroy() {
		// TODO Auto-generated method stub
		super.onDestroy();
		unbindService(mNClientConnection);
	}

    public void hideSoftKeyboard() {
        if (getCurrentFocus() != null) {
            InputMethodManager inputMethodManager = (InputMethodManager) getSystemService(INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(getCurrentFocus().getWindowToken(), 0);
        }
    }

	public static final int PROGRESS_LOAD		= 0;
	public static final int PROGRESS_LOGIN		= 1;
	public static final int PROGRESS_REGISTER	= 2;
	private View mFormView;
	private View mStatusView;
	private TextView mStatusMessageView;
	boolean curProgress;
	@TargetApi(Build.VERSION_CODES.HONEYCOMB_MR2)
	public void showProgress(final boolean show, int type) {
		if (mStatusView == null)
			return;
		curProgress = show;
		if (show)
			switch (type) {
			case PROGRESS_LOAD:
				mStatusMessageView.setText(R.string.progress_loading);
				break;
			case PROGRESS_LOGIN:
				mStatusMessageView.setText(R.string.progress_login);
				break;
			case PROGRESS_REGISTER:
				mStatusMessageView.setText(R.string.progress_register);
				break;
			}
			
		// On Honeycomb MR2 we have the ViewPropertyAnimator APIs, which allow
		// for very easy animations. If available, use these APIs to fade-in
		// the progress spinner.
		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR2) {
			int shortAnimTime = getResources().getInteger(
					android.R.integer.config_shortAnimTime);

			mStatusView.setVisibility(View.VISIBLE);
			mStatusView.animate().setDuration(shortAnimTime)
					.alpha(show ? 1 : 0)
					.setListener(new AnimatorListenerAdapter() {
						@Override
						public void onAnimationEnd(Animator animation) {
							mStatusView.setVisibility(show ? View.VISIBLE
									: View.GONE);
						}
					});

			mFormView.setVisibility(View.VISIBLE);
			mFormView.animate().setDuration(shortAnimTime)
					.alpha(show ? 0 : 1)
					.setListener(new AnimatorListenerAdapter() {
						@Override
						public void onAnimationEnd(Animator animation) {
							mFormView.setVisibility(show ? View.GONE
									: View.VISIBLE);
						}
					});
		} else {
			// The ViewPropertyAnimator APIs are not available, so simply show
			// and hide the relevant UI components.
			mStatusView.setVisibility(show ? View.VISIBLE : View.GONE);
			mFormView.setVisibility(show ? View.GONE : View.VISIBLE);
		}
	}
    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();
        if (id == R.id.action_reset_password) {
        	openResetFragment();
            return true;
        }
          return super.onOptionsItemSelected(item);
    }
	
	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		super.onCreateOptionsMenu(menu);
		getMenuInflater().inflate(R.menu.login, menu);
//		return false;
		return true;
	}
}
