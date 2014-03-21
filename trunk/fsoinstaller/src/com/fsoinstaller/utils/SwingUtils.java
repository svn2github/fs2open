/*
 * This file is part of the FreeSpace Open Installer
 * Copyright (C) 2013 The FreeSpace 2 Source Code Project
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

package com.fsoinstaller.utils;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;

import java.awt.Dimension;
import java.awt.EventQueue;
import java.awt.Toolkit;
import java.awt.Window;
import java.io.File;
import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.atomic.AtomicReference;

import javax.swing.JFileChooser;
import javax.swing.filechooser.FileFilter;


/**
 * Useful methods for working with Swing.
 * 
 * @author Goober5000
 */
public class SwingUtils
{
	private static Logger logger = Logger.getLogger(SwingUtils.class);
	
	/**
	 * Prevent instantiation.
	 */
	private SwingUtils()
	{
	}
	
	public static void centerWindowOnScreen(Window window)
	{
		if (window == null)
		{
			logger.warn("Window is null!");
			return;
		}
		
		// calculate the coordinates to center the window
		Dimension screenSize = Toolkit.getDefaultToolkit().getScreenSize();
		int x = (int) ((screenSize.getWidth() - window.getWidth()) / 2.0 + 0.5);
		int y = (int) ((screenSize.getHeight() - window.getHeight()) / 2.0 + 0.5);
		
		// make sure it isn't off the top-left of the screen
		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		
		// center it
		window.setLocation(x, y);
	}
	
	public static void centerWindowOnParent(Window window, Window parent)
	{
		if (parent == null)
		{
			logger.warn("Centering " + ((window == null) ? null : window.getName()) + " on null parent!");
			centerWindowOnScreen(window);
			return;
		}
		
		// calculate the coordinates to center the window
		int x = (int) (parent.getX() + ((parent.getWidth() - window.getWidth()) / 2.0 + 0.5));
		int y = (int) (parent.getY() + ((parent.getHeight() - window.getHeight()) / 2.0 + 0.5));
		
		// make sure it isn't off the top-left of the screen
		if (x < 0)
			x = 0;
		if (y < 0)
			y = 0;
		
		// center it
		window.setLocation(x, y);
	}
	
	public static void invokeAndWait(Runnable runnable)
	{
		try
		{
			// make sure we don't hang up the event thread
			if (EventQueue.isDispatchThread())
				runnable.run();
			else
				EventQueue.invokeAndWait(runnable);
		}
		catch (InvocationTargetException ite)
		{
			logger.error("The invocation threw an exception!", ite);
			if (ite.getCause() instanceof Error)
				throw (Error) ite.getCause();
			else if (ite.getCause() instanceof RuntimeException)
				throw (RuntimeException) ite.getCause();
			else
				throw new IllegalStateException("Unrecognized invocation exception!", ite.getCause());
		}
		catch (InterruptedException ie)
		{
			logger.error("The invocation was interrupted!", ie);
			Thread.currentThread().interrupt();
		}
	}
	
	public static File promptForFile(final String dialogTitle, final File applicationDir, String ... filterInfo)
	{
		if (filterInfo.length % 2 != 0)
			throw new IllegalArgumentException("The filterInfo parameter must be divisible by two!");
		
		// the varargs parameter repeats extension and description for as many filters as we need
		List<FileFilter> filterList = new ArrayList<FileFilter>();
		for (int i = 0; i < filterInfo.length; i += 2)
		{
			final String filterExtension = filterInfo[i];
			final String filterDescription = filterInfo[i + 1];
			
			FileFilter filter = new FileFilter()
			{
				@Override
				public boolean accept(File path)
				{
					if (path.isDirectory())
						return true;
					String name = path.getName();
					int pos = name.lastIndexOf('.');
					if (pos < 0)
						return false;
					return name.substring(pos + 1).equalsIgnoreCase(filterExtension);
				}
				
				@Override
				public String getDescription()
				{
					return filterDescription;
				}
			};
			filterList.add(filter);
		}
		
		final List<FileFilter> filters = Collections.unmodifiableList(filterList);
		final AtomicReference<File> fileHolder = new AtomicReference<File>();
		
		// must go on the event thread, ugh...
		SwingUtils.invokeAndWait(new Runnable()
		{
			public void run()
			{
				// create a file chooser
				JFileChooser chooser = new JFileChooser();
				chooser.setDialogTitle(dialogTitle);
				chooser.setCurrentDirectory(applicationDir);
				
				// set filters
				boolean firstFilter = true;
				for (FileFilter filter: filters)
				{
					chooser.addChoosableFileFilter(filter);
					
					if (firstFilter)
					{
						chooser.setFileFilter(filter);
						firstFilter = false;
					}
				}
				
				// display it
				int result = chooser.showDialog(null, XSTR.getString("OK"));
				if (result == JFileChooser.APPROVE_OPTION)
					fileHolder.set(chooser.getSelectedFile());
			}
		});
		
		return fileHolder.get();
	}
}
