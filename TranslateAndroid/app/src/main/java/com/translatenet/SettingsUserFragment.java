package com.translatenet;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.annotation.TargetApi;
import android.opengl.Visibility;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.FrameLayout.LayoutParams;

public class SettingsUserFragment extends Fragment {
	User user;
	boolean DonePressed;
	public static final int NameMinLength = 2;
	public static final String NameSymbols = ".,-_";
	public void setUser(User _user) {
		if (_user == null)
			return;
		try {
			user = (User)_user.clone();
		} catch (Exception ex) {
		}
		UpdateUserView(user);
	}
	public boolean onPacketErrorUserData(int code) {
		activity.showProgress(false);
		switch (code) {
		case Parser.ERROR_NOERROR:
            if (user.isT) {
                TextView PayPalEmailTV = (TextView) getView().findViewById(R.id.SU_PayPalEmailTextView);
                if (PayPalEmailTV != null)
                    PayPalEmailTV.setText(user.paypal_email);
            }
    		activity.setUser(user);
    		if (DonePressed)
    			activity.CloseSettingsUser();
//			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.user_success), Toast.LENGTH_LONG);
//			toast.show();
            return true;
        case Parser.ERROR_NAME_EXIST:
            setUser(activity.getUser());
            EditText NameEditText = (EditText)getView().findViewById(R.id.SU_NameEditText);
            activity.EditTextSetError(NameEditText, activity.getErrorMessage(activity.getApplicationContext(), code));
            return true;
		default:
			setUser(activity.getUser());
			activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), code));
            return true;
		}
	}
	void UpdateUserView(User user) {
		if (user == null || getView() == null)
			return;
		TextView nameTV = (TextView)getView().findViewById(R.id.SU_NameEditText);
		if (nameTV != null) {
            if (user.name != null) {
                nameTV.setText(user.name);
                if (user.name.length() > 0)
                    nameTV.setEnabled(false);
            }
            if (user.name == null || user.name.length() == 0) {
                TextView nameDescrTV = (TextView) getView().findViewById(R.id.SU_NameDescrTextView);
                if (nameDescrTV != null) {
                    nameDescrTV.setText(getString(R.string.name_descr) + "*");
                }
            }
		}
        if (user.email != null) {
            TextView emailTV = (TextView) getView().findViewById(R.id.SU_EmailTextView);
            if (emailTV != null)
                emailTV.setText(user.email);
        }
		TextView balanceTV = (TextView)getView().findViewById(R.id.SU_BalanceTextView);
		if (balanceTV != null)
			balanceTV.setText(activity.FormatPrice(user.balance));
        if (user.paypal_email != null) {
            TextView PayPalEmailTV = (TextView) getView().findViewById(R.id.SU_PayPalEmailTextView);
            PayPalEmailTV.setText(user.paypal_email);
        }
	}
	String currentBilling;
	void BillingDone(String item, boolean success, String data, String signature) {
		if (!success) {
			item = null;
			return;
		}
		if (item == null) {
			activity.UserMessage(getString(R.string.error), getString(R.string.error_billing));
			return;
		}
		int money;
		if (item.compareToIgnoreCase(MainActivity.Billing_1credit) == 0)
			money = 100;
        else if (item.compareToIgnoreCase(MainActivity.Billing_2credit) == 0)
            money = 200;
		else if (item.compareToIgnoreCase(MainActivity.Billing_5credit) == 0)
			money = 500;
        else if (item.compareToIgnoreCase(MainActivity.Billing_10credit) == 0)
            money = 1000;
		else if (item.compareToIgnoreCase(MainActivity.Billing_25credit) == 0)
			money = 2500;
		else {
			currentBilling = null;
			activity.UserMessage(getString(R.string.error), getString(R.string.error_billing));
			return;
		}
		currentBilling = null;
		activity.sendPacket_Billing(money, data, signature);
	}
	public boolean BillingPacketError(int code) {
		if (code != Parser.ERROR_NOERROR) {
			activity.UserMessage(getString(R.string.error_billing), activity.getErrorMessage(activity.getApplicationContext(), code));
		} else {
            activity.UserMessage(getString(R.string.note), activity.getString(R.string.paypal_transfer_success));
        }
		DonePressed = false;
		activity.sendPacket_GetUserData();
        return true;
	}
	public boolean PayPalTransferPacketError(int code) {
		if (code != Parser.ERROR_NOERROR) {
			activity.UserMessage(getString(R.string.error_paypal), activity.getErrorMessage(activity.getApplicationContext(), code));
			return true;
		}
		DonePressed = false;
		activity.sendPacket_GetUserData();
        return true;
	}
	boolean checkName(String name) {
		if (name.length() < NameMinLength)
			return false;
		for (int i=0; i<name.length(); i++)
			if ((name.charAt(i) < 'a' || name.charAt(i) > 'z') &&
				(name.charAt(i) < 'A' || name.charAt(i) > 'Z') &&
				(name.charAt(i) < '0' || name.charAt(i) > '9')) {
				int j = 0;
				for (; j<NameSymbols.length(); j++)
					if (name.charAt(i) == NameSymbols.charAt(j))
						break;
				if (j == NameSymbols.length())
					return false;
			}
		return true;
	}
	public static class PayPalDialogFragment extends DialogFragment {
		SettingsUserFragment fragment;
		String paypalEmail;
		public void setFragment(SettingsUserFragment _fragment) {
			fragment = _fragment;
		}
		@Override
		public void onStart() {
			// TODO Auto-generated method stub
			super.onStart();
    		getDialog().setTitle(getString(R.string.paypal_acc));
		}
		@Override
		public View onCreateView(LayoutInflater inflater, ViewGroup container,
				Bundle savedInstanceState) {
			// TODO Auto-generated method stub
			View v = inflater.inflate(R.layout.fragment_settings_paypal_dialog, container, false);
	        Button CancelButton = (Button)v.findViewById(R.id.SPP_CancelButton);
	        CancelButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	        		dismiss();
	        	}
	        });
	        Button DoneButton = (Button)v.findViewById(R.id.SPP_DoneButton);
	        DoneButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	        		EditText emailET = (EditText)getView().findViewById(R.id.SPP_PayPalEditText);
	        		String email = emailET.getText().toString();
	        		if (!MainActivity.checkEmail(email)) {
                        MainActivity.EditTextSetError(emailET, getString(R.string.invalid_email));
	        			return;
	        		}
	        		fragment.onEnteredPayPalEmail(email);
	        		dismiss();
	        	}
	        });
			return v;
		}
	};

    public void onEnteredPayPalEmail(String email) {
        if (email == null)
            return;
        if (activity.getUser().paypal_email == null ||
                !email.equalsIgnoreCase(activity.getUser().paypal_email)) {
/*            if (user.name == null || user.name.length() == 0)
                user.name = getName();
            if (user.name == null)
                return;
 */
            activity.showProgress(true);
            user.paypal_email = email;
            DonePressed = false;
            if (user.name == null || user.name.length() == 0)
                user.name = getName();
            activity.sendPacket_UserData(user);
        }
    }
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		DonePressed = false;
		RelativeLayout mainLayout = (RelativeLayout)getView().findViewById(R.id.SU_form);
		RelativeLayout billingLayout = (RelativeLayout)getView().findViewById(R.id.SU_BillingLayout);
		RelativeLayout paypalLayout = (RelativeLayout)getView().findViewById(R.id.SU_PayPalLayout);
		if (user != null) {
			UpdateUserView(user);
			
			RelativeLayout.LayoutParams lpLayout = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
			Button DoneButton = (Button)getView().findViewById(R.id.SU_DoneButton);
			RelativeLayout.LayoutParams lpDone = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
			if (user.isT) {
                billingLayout.setVisibility(View.GONE);
                lpLayout.addRule(RelativeLayout.BELOW, mainLayout.getId());
				paypalLayout.setVisibility(View.VISIBLE);
				paypalLayout.setLayoutParams(lpLayout);
                lpDone.addRule(RelativeLayout.BELOW, paypalLayout.getId());
                lpDone.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                DoneButton.setLayoutParams(lpDone);
			} else {
                paypalLayout.setVisibility(View.GONE);
                lpLayout.addRule(RelativeLayout.BELOW, mainLayout.getId());
				billingLayout.setVisibility(View.VISIBLE);
				billingLayout.setLayoutParams(lpLayout);
                lpDone.addRule(RelativeLayout.BELOW, billingLayout.getId());
                lpDone.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
                DoneButton.setLayoutParams(lpDone);
			}
		} else {
			billingLayout.setVisibility(View.GONE);
			paypalLayout.setVisibility(View.GONE);			
		}
	}

    String getName() {
        TextView nameTV = (TextView)getView().findViewById(R.id.SU_NameEditText);
        if (nameTV == null)
            return null;
        String name = nameTV.getText().toString();
        if (!checkName(name)) {
            MainActivity.TextViewSetError(nameTV, getString(R.string.invalid_name) + NameSymbols);
            return null;
        }
        return name;
    }

	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_settings_user, container, false);

        v.setOnTouchListener(new View.OnTouchListener() {
            public boolean onTouch(View v, MotionEvent event) {
                InputMethodManager imm = (InputMethodManager)activity.getSystemService(activity.getApplicationContext().
                        INPUT_METHOD_SERVICE);
                imm.hideSoftInputFromWindow(activity.getCurrentFocus().getWindowToken(), 0);
                return true;
            }
        });
        Button Billing1Button = (Button)v.findViewById(R.id.SU_Billing1Button);
        Billing1Button.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		currentBilling = MainActivity.Billing_1credit;
        		activity.BillingRequestAddMoney(MainActivity.Billing_1credit);
        	}
        });
        Button Billing2Button = (Button)v.findViewById(R.id.SU_Billing2Button);
        Billing2Button.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                currentBilling = MainActivity.Billing_2credit;
                activity.BillingRequestAddMoney(MainActivity.Billing_2credit);
            }
        });
        Button Billing5Button = (Button)v.findViewById(R.id.SU_Billing5Button);
        Billing5Button.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                currentBilling = MainActivity.Billing_5credit;
                activity.BillingRequestAddMoney(MainActivity.Billing_5credit);
            }
        });
        Button Billing10Button = (Button)v.findViewById(R.id.SU_Billing10Button);
        Billing10Button.setOnClickListener(new OnClickListener()  {
            @Override
            public void onClick(View v) {
                currentBilling = MainActivity.Billing_10credit;
                activity.BillingRequestAddMoney(MainActivity.Billing_10credit);
            }
        });
        Button Billing25Button = (Button)v.findViewById(R.id.SU_Billing25Button);
        Billing25Button.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		currentBilling = MainActivity.Billing_25credit;
        		activity.BillingRequestAddMoney(MainActivity.Billing_25credit);
        	}
        });
        
        Button PayPalEditButton = (Button)v.findViewById(R.id.SU_PayPalEditButton);
        PayPalEditButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
       			PayPalDialogFragment paypalFragment = new PayPalDialogFragment();
       			paypalFragment.setFragment(SettingsUserFragment.this);
       			paypalFragment.show(getFragmentManager(), "paypal_dialog");
        	}
        });
        Button PayPalTransferButton = (Button)v.findViewById(R.id.SU_PayPalTransferButton);
        PayPalTransferButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
                if (user.balance <= 0)
                    return;
        		activity.sendPacket_PayPalTransfer(user.balance);
        	}
        });
        Button DoneButton = (Button)v.findViewById(R.id.SU_DoneButton);
        DoneButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null || activity.getUser() == null)
        			return;
                if (user.name == null || user.name.length() == 0) {
                    user.name = getName();
                    if (user.name == null || user.name.length() == 0)
                        return;
                }
        		if (!user.name.equals(activity.getUser().name)) {
        			activity.showProgress(true);
        			activity.sendPacket_UserData(user);
        			DonePressed = true;
        		} else {
        			activity.CloseSettingsUser();
        		}
        	}
        });
		return v;
	}
	MainActivity activity;
	public void init(MainActivity _activity) {
		activity = _activity;
	}
}
