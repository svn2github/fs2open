/*
 * This file is part of the FreeSpace Open Installer
 * Copyright (C) 2010 The FreeSpace 2 Source Code Project
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

package com.fsoinstaller.wizard;

import java.awt.EventQueue;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JPanel;
import javax.swing.JProgressBar;

import com.fsoinstaller.internet.DownloadEvent;
import com.fsoinstaller.internet.DownloadListener;
import com.fsoinstaller.internet.Downloader;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;


public class DownloadPanel extends JPanel implements DownloadListener
{
	private final JProgressBar progressBar;
	private final StoplightPanel stoplightPanel;
	private Downloader downloader;
	
	public DownloadPanel()
	{
		super();
		
		this.progressBar = new JProgressBar(0, GUIConstants.BAR_MAXIMUM);
		progressBar.setStringPainted(true);
		
		this.stoplightPanel = new StoplightPanel((int) progressBar.getPreferredSize().getHeight());
		
		setBorder(BorderFactory.createEmptyBorder(GUIConstants.SMALL_MARGIN, GUIConstants.SMALL_MARGIN, GUIConstants.SMALL_MARGIN, 0));
		setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
		add(progressBar);
		add(Box.createHorizontalStrut(GUIConstants.SMALL_MARGIN));
		add(stoplightPanel);
		
		// set up the initial GUI display without actually having a downloader yet
		setDownloader(null);
	}
	
	public Downloader getDownloader()
	{
		return downloader;
	}
	
	public void setDownloader(Downloader downloader)
	{
		if (!EventQueue.isDispatchThread())
			throw new IllegalStateException("Must be called on the event-dispatch thread!");
		
		if (this.downloader != null)
			this.downloader.removeDownloadListener(this);
		
		this.downloader = downloader;
		
		if (this.downloader != null)
			this.downloader.addDownloadListener(this);
		
		progressBar.setString(XSTR.getString("progressBarWaiting"));
		progressBar.setIndeterminate(true);
		progressBar.setValue(0);
		
		stoplightPanel.setPending();
	}
	
	public void downloadAboutToStart(DownloadEvent event)
	{
		progressBar.setString(String.format(XSTR.getString("progressBarStatus"), event.getDownloadName(), 0, event.getTotalBytes()));
		progressBar.setIndeterminate(false);
		progressBar.setValue(0);
		
		stoplightPanel.setPending();
	}
	
	public void downloadProgressReport(DownloadEvent event)
	{
		progressBar.setString(String.format(XSTR.getString("progressBarStatus"), event.getDownloadName(), event.getDownloadedBytes(), event.getTotalBytes()));
		progressBar.setIndeterminate(false);
		progressBar.setValue((int) ((double) event.getDownloadedBytes() / event.getTotalBytes() * GUIConstants.BAR_MAXIMUM));
	}
	
	public void downloadNotNecessary(DownloadEvent event)
	{
		progressBar.setString(((event == null) ? "" : (event.getDownloadName() + ": ")) + XSTR.getString("progressBarUpToDate"));
		progressBar.setIndeterminate(false);
		progressBar.setValue(GUIConstants.BAR_MAXIMUM);
		
		stoplightPanel.setSuccess();
	}
	
	public void downloadComplete(DownloadEvent event)
	{
		progressBar.setString(((event == null) ? "" : (event.getDownloadName() + ": ")) + XSTR.getString("progressBarComplete"));
		progressBar.setIndeterminate(false);
		progressBar.setValue(GUIConstants.BAR_MAXIMUM);
		
		stoplightPanel.setSuccess();
	}
	
	public void downloadFailed(DownloadEvent event)
	{
		progressBar.setString(((event == null) ? "" : (event.getDownloadName() + ": ")) + XSTR.getString("progressBarFailed"));
		progressBar.setIndeterminate(false);
		progressBar.setValue(0);
		
		stoplightPanel.setFailure();
	}
	
	public void downloadCancelled(DownloadEvent event)
	{
		progressBar.setString(((event == null) ? "" : (event.getDownloadName() + ": ")) + XSTR.getString("progressBarCancelled"));
		progressBar.setIndeterminate(false);
		progressBar.setValue(0);
		
		stoplightPanel.setFailure();
	}
}
