package com.translatenet;

import java.util.ArrayList;

public class Country {
	String name;
	String code;
	String lang;
	String iso3;
	String langs;
	public final static String UNKNOWN = "--";
	public static ArrayList<Country> Countries = new ArrayList<Country>();
	public static Country getCountryByCode(String code) {
		if (code == null)
			return null;
		for (int i=0; i<Countries.size(); i++) {
			Country c = Countries.get(i);
			if (c.code.equalsIgnoreCase(code))
				return c;
		}
		return null;
	}
	public static Country getCountryByName(String name) {
		if (name == null)
			return null;
		for (int i=0; i<Countries.size(); i++) {
			Country c = Countries.get(i);
			if (c.name.equalsIgnoreCase(name))
				return c;
		}
		return null;
	}
	public static String CountryToCode(String name) {
		if (name == null)
			return Country.UNKNOWN;
		for (int i=0; i<Countries.size(); i++) {
			Country c = Countries.get(i);
			if (c.name.equalsIgnoreCase(name))
				return c.code; 
		}
		return Country.UNKNOWN;
	}
	public static String CodeToCountry(String code) {
		if (code == null)
			return CodeToCountry(Country.UNKNOWN);
		for (int i=0; i<Countries.size(); i++) {
			Country c = Countries.get(i);
			if (c.code.equalsIgnoreCase(code))
				return c.name; 
		}
		return CodeToCountry(Country.UNKNOWN);
	}
	public static boolean isCountry(String country) {
		if (country == null)
			return false;
		for (int i=0; i<Countries.size(); i++)
			if (Countries.get(i).code.compareToIgnoreCase(country) == 0)
				return true;
		return false;
	}
	public static String getCountryLang(String country) {
		if (country == null)
			return Lang.DEFAULT;
		Country c = getCountryByCode(country);
		if (c == null)
			return Lang.DEFAULT;
		return c.lang;
	}
}
