package com.translatenet;

import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.View.OnClickListener;
import android.widget.Button;
import android.view.MotionEvent;
import android.graphics.Color;

public class SettingsFragment extends Fragment {
	MainActivity activity = null;
	public void init(MainActivity act) {
		activity = act;
	}
	@Override
	public void onCreate(Bundle savedInstanceState) {
		// TODO Auto-generated method stub
		super.onCreate(savedInstanceState);
	}
	@Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.fragment_settings, container, false);
		Button b = (Button)v.findViewById(R.id.langsButton);
		b.setOnClickListener(new OnClickListener()  {
			@Override
			public void onClick(View v) {
				if (activity != null)
					activity.OpenSettingsLang();
			}
		});
		b.setOnTouchListener(new View.OnTouchListener() {          
		    @Override
		    public boolean onTouch(View view, MotionEvent event) {
		        switch (event.getAction())
		        {
		            case MotionEvent.ACTION_DOWN:    
		                ((Button)view).setBackgroundColor(Color.LTGRAY);
		                break;
		            case MotionEvent.ACTION_UP:
		                ((Button)view).setBackgroundColor(Color.TRANSPARENT);
		        }
		        return false;
		    }
		});
       return v;
    }

}
