package com.translatenet;

import android.content.Context;
import android.location.Location;
import android.location.LocationListener;
import android.location.LocationManager;
import android.os.Bundle;
import android.util.Log;
import android.util.Xml;

import com.translatenet.TNHttpRequest;

import org.xmlpull.v1.XmlPullParser;

import java.io.StringReader;
import java.util.Date;
import java.util.Locale;

/**
 * Created by roman on 27.05.15.
 */
public class TNGeoLocation implements TNHttpResponse {
    MainActivity activity;
    LocationManager locationManager;
    double lon, lat;
    double lonBase, latBase;
    boolean LocationRequested;
    Date lastRequest = null;
    public final static int GEO_HTTP_TIMEOUT = 300000;

    public boolean isLocationServiceEnabled() {
        if (locationManager == null)
            return false;
        if (!locationManager.isProviderEnabled(LocationManager.NETWORK_PROVIDER))
            return false;
        return true;
    }

    public TNGeoLocation(MainActivity _activity) {
        lon = lat = -1;
        lonBase = latBase = -1;
        activity = _activity;
        locationManager = (LocationManager)activity.getSystemService(Context.LOCATION_SERVICE);
// Define a listener that responds to location updates
        LocationListener locationListener = new LocationListener() {
            public void onLocationChanged(Location location) {
                // Called when a new location is found by the network location provider.
                lat = location.getLatitude();
                lon = location.getLongitude();
                if (latBase < 0 || lonBase < 0) {
                    latBase = lat;
                    lonBase = lon;
                }
                if (Math.abs(latBase - lat) > 1 || Math.abs(lonBase - lon) > 1)
                    LocationRequested = true;
                if (LocationRequested) {
                    if (lastRequest == null || System.currentTimeMillis() > lastRequest.getTime() + GEO_HTTP_TIMEOUT) {
                        lastRequest = new Date(System.currentTimeMillis());
                        requestCountryByCoordinates(lon, lat);
                    }
                }
            }

            public void onStatusChanged(String provider, int status, Bundle extras) {}

            public void onProviderEnabled(String provider) {}

            public void onProviderDisabled(String provider) {}
        };

        Location lastL = locationManager.getLastKnownLocation(LocationManager.NETWORK_PROVIDER);
        lat = latBase = lastL.getLatitude();
        lon = lonBase = lastL.getLongitude();
// Register the listener with the Location Manager to receive location updates
        locationManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 0, 0, locationListener);
    }

    private XmlPullParser xpp;
    protected class GoogleAddrComp {
        public String type;
        public String short_name;
    }

    String parseCountry() throws Exception {
        String type;
        int eventType = xpp.getEventType();
        int depth = xpp.getDepth();
        do {
            type = xpp.getName();
            if (eventType == XmlPullParser.START_TAG) {
                if (type.equalsIgnoreCase("address_component")) {
                    GoogleAddrComp gac = new GoogleAddrComp();
                    int gacdepth = xpp.getDepth();
                    boolean isCountry = false;
                    do {
                        if (xpp.getEventType() == XmlPullParser.START_TAG) {
                            if (xpp.getName().compareToIgnoreCase("type") == 0) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    gac.type = xpp.getText();
                            } else if (xpp.getName().compareToIgnoreCase("short_name") == 0) {
                                if (xpp.next() == XmlPullParser.TEXT)
                                    gac.short_name = xpp.getText();
                            }
                        }
                        if (gac.type != null && gac.type.equalsIgnoreCase("country"))
                            isCountry = true;
                        xpp.next();
                        while (xpp.getEventType() == XmlPullParser.TEXT)
                            xpp.next();
                    } while (xpp.getDepth() > gacdepth);
                    if (gac.short_name != null) {
                        if (isCountry)
                            return gac.short_name.toLowerCase(Locale.UK);
                    }

                }
            }
            eventType = xpp.next();
        } while (xpp.getDepth() >= depth);
        return Country.UNKNOWN;
    }

    String parseCountryFromResponse(String xml) {
        xpp = Xml.newPullParser();
        try {
            xpp.setFeature(XmlPullParser.FEATURE_PROCESS_NAMESPACES, true);
        } catch (Exception ex) {
        }
        String type;
        try {
            xpp.setInput(new StringReader(xml));
            int eventType = xpp.getEventType();
            while (eventType != XmlPullParser.END_DOCUMENT) {
                type = xpp.getName();
                if (eventType == XmlPullParser.START_TAG) {
                    if (type.equalsIgnoreCase("GeocodeResponse")) {
                        return parseCountry();
                    }
                }
                eventType = xpp.next();
            }
        } catch (Exception ex) {
            return Country.UNKNOWN;
        }
        return Country.UNKNOWN;
    }

    public void processHttpResponse(String result)
    {
        LocationRequested = false;
        latBase = lat;
        lonBase = lon;
        String c = parseCountryFromResponse(result);
        if (Country.isCountry(c))
            activity.onGotCurrentCountry(c);
        else
            activity.onGotCurrentCountry(Country.UNKNOWN);
    }

    public void requestCountryByCoordinates(double lon, double lat) {
        if (lat < 0 || lon < 0)
            return;
        String request = "http://maps.googleapis.com/maps/api/geocode/xml?latlng=";
        request += lat;
        request += ",";
        request += lon;

        TNHttpRequest httpReq = new TNHttpRequest(this);
        httpReq.execute(request);
    }

    public void requestCurrentCountry() {
        if (lat < 0 || lon < 0)
            LocationRequested = true;
        else
            requestCountryByCoordinates(lon, lat);
    }
}
