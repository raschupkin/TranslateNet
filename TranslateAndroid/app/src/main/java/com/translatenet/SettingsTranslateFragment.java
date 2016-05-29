package com.translatenet;

import java.util.Iterator;
import android.widget.EditText;
import android.widget.LinearLayout;
import java.util.Map;

import com.translatenet.MainActivity.LangCallback;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.widget.FrameLayout.LayoutParams;
import android.support.v4.app.DialogFragment;

public class SettingsTranslateFragment extends Fragment {
	User translator;
	public final static int DEFAULT_PRICE = 100;
	public void setTranslator(User _translator) {
		if (_translator == null)
			return;
		try {
			translator = (User)_translator.clone();
		} catch (Exception ex) {
		}
	}
	class TranslateItem {
		String lang;
		int price;
		RelativeLayout layout;
		TextView LangTV;
		TextView PriceTV;
		Button ChangeButton;
		Button DeleteButton;
		public void UpdateView() {
			if (lang == null || LangTV == null || PriceTV == null)
				return;
			LangTV.setText(activity.getLangNameByCode(lang));		
			PriceTV.setText(activity.FormatPrice(price) + "/" + getString(R.string.min));
		}
	}
	public void onChangeTranslateItemDialog(String lang, int price) {
		PriceDialogFragment priceFragment = new PriceDialogFragment();
		priceFragment.setFragment(SettingsTranslateFragment.this);
		priceFragment.setLang(lang, price);
		priceFragment.show(getFragmentManager(), "price_dialog");
	}
	public void onDeleteTranslateItem(TranslateItem ti) {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
		if (ll == null)
			return;
		ll.removeView(ti.layout);
	}
	public boolean onPacketErrorUserData(int code) {
		activity.showProgress(false);
		switch (code) {
		case Parser.ERROR_NOERROR:
	   		activity.setUser(translator);
	   		activity.CloseSettingsTranslate();
//			Toast toast = Toast.makeText(activity.getApplicationContext(), getString(R.string.translate_success), Toast.LENGTH_LONG);
//			toast.show();
            return true;
	   	default:
			setTranslator(activity.getUser());
			activity.UserMessage(getString(R.string.error), activity.getErrorMessage(activity.getApplicationContext(), code));
            ClearTranslateList();
            CreateTranslateList();
			return true;
		}
 	}
	void CreateTranslateList() {
		if (getView() == null)
			return;
		LinearLayout list = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
        if (list == null)
            return;
		Iterator it = translator.translate.entrySet().iterator();
		while (it.hasNext()) {
			Map.Entry<String, Integer> t = (Map.Entry<String, Integer>)it.next();
			TranslateItem ti = createTranslateItem(t.getKey(), t.getValue());
			list.addView(ti.layout);
			ti.UpdateView();
		}
			
	}
    void ClearTranslateList() {
        if (getView() == null)
            return;
        LinearLayout list = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
        if (list == null)
            return;
        list.removeAllViews();
    }

	static int free_id = 1;
	// Returns a valid id that isn't in use
	public int generateId(){  
	    View v = getView().findViewById(free_id);  
	    while (v != null){  
	        v = getView().findViewById(++free_id);  
	    }  
	    return free_id++;  
	}

	public TranslateItem addTranslatorItem(String lang, int price) {
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
		if (ll == null)
			return null;
		for (int j=0; j<ll.getChildCount(); j++) {
			TranslateItem item = (TranslateItem)ll.getChildAt(j).getTag();
			if (item == null) 
				continue;
			if (item.lang == lang)
				return null;
		}
		TranslateItem ti = createTranslateItem(lang, price);
		ll.addView(ti.layout);
		ti.UpdateView();
		translator.translate.put(lang, price);
		return ti;
	}
	public void updatePrice(String lang, int price) { 
		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
		if (ll == null)
			return;
		for (int j=0; j<ll.getChildCount(); j++) {
			TranslateItem ti = (TranslateItem)ll.getChildAt(j).getTag();
			if (ti == null) 
				continue;
			if (ti.lang == lang) {
				ti.price = price;
				ti.UpdateView();
				translator.translate.put(lang, price);
			}
		}
	}
	private static int padding_vert = 5;
	private static int padding_hor = 5;
	protected TranslateItem createTranslateItem(String lang, int price) {
		TranslateItem ti = new TranslateItem();
		ti.lang = lang;
		ti.price = price;
		ti.layout = new RelativeLayout(getView().getContext());

		TextView LangLabelTV = new TextView(getView().getContext());
		LangLabelTV.setId(generateId());
		RelativeLayout.LayoutParams lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		LangLabelTV.setLayoutParams(lp);
		LangLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		LangLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		LangLabelTV.setText(getString(R.string.lang));
		ti.layout.addView(LangLabelTV);

		ti.LangTV = new TextView(getView().getContext());
		ti.LangTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.RIGHT_OF, LangLabelTV.getId());
		ti.LangTV.setLayoutParams(lp);
		ti.LangTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		ti.LangTV.setPadding(padding_hor, padding_vert, 0, 0);
		ti.layout.addView(ti.LangTV);

		TextView PriceLabelTV = new TextView(getView().getContext());
		PriceLabelTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, LangLabelTV.getId());
		PriceLabelTV.setLayoutParams(lp);
		PriceLabelTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		PriceLabelTV.setPadding(padding_hor, padding_vert, 0, 0);
		PriceLabelTV.setText(getString(R.string.price) + ": ");
		ti.layout.addView(PriceLabelTV);

		ti.PriceTV = new TextView(getView().getContext());
		ti.PriceTV.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, LangLabelTV.getId());
		lp.addRule(RelativeLayout.RIGHT_OF, PriceLabelTV.getId());
		ti.PriceTV.setLayoutParams(lp);
		ti.PriceTV.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		ti.PriceTV.setPadding(padding_hor, padding_vert, 0, 0);
		ti.layout.addView(ti.PriceTV);

		ti.ChangeButton = new Button(getView().getContext());
		ti.ChangeButton.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.BELOW, PriceLabelTV.getId());
		ti.ChangeButton.setLayoutParams(lp);
		ti.ChangeButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		ti.ChangeButton.setPadding(padding_hor, padding_vert, 0, padding_vert*3);
		ti.ChangeButton.setText(getString(R.string.update));
		ti.layout.addView(ti.ChangeButton);
        ti.ChangeButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
        		if (ll == null)
        			return;
        		for (int j=0; j<ll.getChildCount(); j++) {
        			TranslateItem ti = (TranslateItem)ll.getChildAt(j).getTag();
        			if (ti == null) 
        				continue;
        			if (ti.ChangeButton == v)
        				onChangeTranslateItemDialog(ti.lang, ti.price);
        		}
        	}
        });

		ti.DeleteButton = new Button(getView().getContext());
		ti.DeleteButton.setId(generateId());
		lp = new RelativeLayout.LayoutParams(LayoutParams.WRAP_CONTENT, LayoutParams.WRAP_CONTENT);
		lp.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
		lp.addRule(RelativeLayout.BELOW, PriceLabelTV.getId());
		ti.DeleteButton.setLayoutParams(lp);
		ti.DeleteButton.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
		ti.DeleteButton.setPadding(padding_hor, padding_vert, 0, padding_vert*3);
		ti.DeleteButton.setText(getString(R.string.remove));
		ti.layout.addView(ti.DeleteButton);
        ti.DeleteButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		LinearLayout ll = (LinearLayout)getView().findViewById(R.id.ST_TranslateListLayout);
        		if (ll == null)
        			return;
        		for (int j=0; j<ll.getChildCount(); j++) {
        			TranslateItem ti = (TranslateItem)ll.getChildAt(j).getTag();
        			if (ti == null) 
        				continue;
        			if (ti.DeleteButton == v) {
        				onDeleteTranslateItem(ti);
        				translator.translate.remove(ti.lang);
        			}
        		}
        	}
        });

		ti.layout.setTag(ti);
		return ti;
	}

    public static class PriceDialogFragment extends DialogFragment {
		SettingsTranslateFragment fragment; 
		String lang;
		int price;
		public void setFragment(SettingsTranslateFragment _fragment) {
			fragment = _fragment;
		}
		void UpdateLangView() {
			if (getView() == null)
				return;
			TextView langTV = (TextView)getView().findViewById(R.id.SPR_LanguageTextView);
			langTV.setText(fragment.getMainActivity().getLangNameByCode(lang));
	        
			EditText dollarsET = (EditText)getView().findViewById(R.id.SPR_DollarsEditText);
	        EditText centsET = (EditText)getView().findViewById(R.id.SPR_CentsEditText);
	        int dollars = price / 100;
	        int cents = price - dollars * 100;
	        dollarsET.setText(Integer.toString(dollars));
	        centsET.setText(Integer.toString(cents));

	        TextView NoteFeeTV = (TextView)getView().findViewById(R.id.SPR_NoteFeeTextView);
	        String fee_text = getString(R.string.note_fee1);
	        fee_text += " " + Integer.toString(fragment.getMainActivity().getOptions().FeeMarket) + "% ";
	        fee_text += getString(R.string.note_fee2);
	        fee_text += " " + Integer.toString(fragment.getMainActivity().getOptions().FeeApp) + "% ";
	        fee_text += getString(R.string.note_fee3);
	        NoteFeeTV.setText(fee_text);
	        
	        TextView NoteAverageTV = (TextView)getView().findViewById(R.id.SPR_NoteAverageTextView);
	        int avg_price = Lang.getAvgPrice(lang);
	        int avg_dollars = avg_price / 100;
	        int avg_cents = avg_price - avg_dollars * 100;
	        String avg_price_text;
	        if (avg_price > 0)
	        	avg_price_text = Integer.toString(avg_dollars) + getString(R.string.credit) + " " +
		        		Integer.toString(avg_cents) + " " + getString(R.string.cents);
	        else
	        	avg_price_text = getString(R.string.not_enough_data);
	        NoteAverageTV.setText(getString(R.string.note_average) + avg_price_text);

	        TextView NoteSiteTV = (TextView)getView().findViewById(R.id.SPR_NoteSiteTextView);
	        String site_text = getString(R.string.note_site);
	        site_text += getString(R.string.site_name);
	        NoteSiteTV.setText(site_text);
		}
		public void setLang(String _lang, int _price) {
			lang = _lang;
			price = _price;
			if (getView() != null)
				UpdateLangView();
		}

		@Override
		public void onStart() {
			// TODO Auto-generated method stub
			super.onStart();
    		getDialog().setTitle(getString(R.string.price_dialog));
			if (lang != null)
				UpdateLangView();
		}
		@Override
		public View onCreateView(LayoutInflater inflater, ViewGroup container,
				Bundle savedInstanceState) {
			// TODO Auto-generated method stub
			View v = inflater.inflate(R.layout.fragment_settings_price_dialog, container, false);
	        Button CancelButton = (Button)v.findViewById(R.id.SPR_CancelButton);
	        CancelButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	        		dismiss();
	        	}
	        });
	        Button DoneButton = (Button)v.findViewById(R.id.SPR_DoneButton);
	        DoneButton.setOnClickListener(new OnClickListener()  {
	        	@Override
	        	public void onClick(View v) {
	    	        EditText dollarsET = (EditText)getView().findViewById(R.id.SPR_DollarsEditText);
	    	        EditText centsET = (EditText)getView().findViewById(R.id.SPR_CentsEditText);
	    	        int dollars = Integer.parseInt(dollarsET.getText().toString());
	    	        int cents = Integer.parseInt(centsET.getText().toString());
	    	        if (dollars < 0 || dollars > 100) {
                        MainActivity.EditTextSetError(dollarsET, getString(R.string.price_error));
	    	        	return;
	    	        }
	    	        if (cents < 0 || cents > 100) {
                        MainActivity.EditTextSetError(centsET, getString(R.string.price_error));
	    	        	return;
	    	        }
	        		int price = dollars * 100 + cents;
	        		if (fragment.addTranslatorItem(lang, price) == null)
	        			fragment.updatePrice(lang, price);
	        		dismiss();
	        	}
	        });
			return v;
		}
	}
	class UpdateLangCallback implements LangCallback { 
		public void LangSelected(String lang) {
			if (!Lang.isLang(lang))
				return;
			if (translator.translate.containsKey(lang)) {
				activity.UserMessage(getString(R.string.error), getString(R.string.lang_dup_error));
				return;
			}
			
			onChangeTranslateItemDialog(lang, DEFAULT_PRICE);
		}		
	}

    @Override
    public void onStart() {
        super.onStart();
        FragmentOpened();
    }

    public void FragmentOpened() {
        if (translator != null) {
            if (getView() ==  null)
                return;
            LinearLayout list = (LinearLayout) getView().findViewById(R.id.ST_TranslateListLayout);
            if (list.getChildCount() == 0)
                CreateTranslateList();
        }
    }

    public void FragmentClosed() {
        ClearTranslateList();
    }

	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_settings_translate, container, false);
        Button AddLangButton = (Button)v.findViewById(R.id.ST_AddButton);
        AddLangButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		activity.SelectLang(null, new UpdateLangCallback(), false);
        	}
        });
        Button DoneButton = (Button)v.findViewById(R.id.ST_DoneButton);
        DoneButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null)
        			return;
        		if (translator.CompareTranslate(activity.getUser().translate)) {
        			activity.CloseSettingsTranslate();
        		} else {
        			activity.showProgress(true);
        			activity.sendPacket_UserData(translator);
        		}
        	}
        });
		return v;
	}
	MainActivity activity;
	public void init(MainActivity _activity) {
		activity = _activity;
	}
    public MainActivity getMainActivity()   {   return activity;    }
}
