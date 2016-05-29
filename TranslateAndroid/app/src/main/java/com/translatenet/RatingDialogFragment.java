package com.translatenet;

import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.text.format.DateUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.EditText;
import android.widget.RatingBar;
import android.widget.TextView;
import android.widget.Button;

import java.text.DateFormat;
import java.util.Date;

public class RatingDialogFragment extends DialogFragment {
	User translator;
	Date date;
	void setTranslator(User _translator, Date _date) {
		if (_translator == null)
			return;
		try {
			translator = (User)_translator.clone();
		} catch (Exception ex) {
		}
		date = _date;
		if (getView() != null)
			UpdateTranslatorView();
	}
	void UpdateTranslatorView() {
		if (getView() == null || translator == null)
			return;
		TextView descrTV = (TextView)getView().findViewById(R.id.RD_Descr);
		descrTV.setText(getString(R.string.rating_descr) + translator.name);
		if (date != null) {
			TextView dateTV = (TextView)getView().findViewById(R.id.RD_Time);
            String text = android.text.format.DateUtils.formatDateTime(activity.getApplicationContext(), date.getTime(),
                    DateUtils.FORMAT_SHOW_DATE|DateUtils.FORMAT_SHOW_TIME|DateUtils.FORMAT_SHOW_YEAR);
//            DateFormat dateFormat = android.text.format.DateFormat.(activity.getApplicationContext());
			//dateTV.setText(dateFormat.format(date));
            dateTV.setText(text);
		}
	}
	@Override
	public void onStart() {
		// TODO Auto-generated method stub
		super.onStart();
		if (translator != null)
			UpdateTranslatorView();
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_rating_dialog, container, false);
        Button cancelButton = (Button)v.findViewById(R.id.RD_CancelButton);
        cancelButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		dismiss();
        	}
        });
        Button doneButton = (Button)v.findViewById(R.id.RD_DoneButton);
        doneButton.setOnClickListener(new OnClickListener()  {
        	@Override
        	public void onClick(View v) {
        		if (activity == null || translator == null)
        			return;
        		RatingBar ratingBar = (RatingBar)getView().findViewById(R.id.RD_RatingBar);
        		int rating = (int)(ratingBar.getRating()*2);
        		activity.sendPacket_MarkRating(translator.id, rating);
        		dismiss();
        	}
        });
        return v;
	}
	MainActivity activity = null;
	public void init(MainActivity act) {
		activity = act;
	}
}
