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

import java.awt.BorderLayout;
import java.awt.EventQueue;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.ScrollPaneConstants;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import javax.swing.event.ChangeEvent;
import javax.swing.event.ChangeListener;

import com.fsoinstaller.common.BaseURL;
import com.fsoinstaller.common.InstallerNode;
import com.fsoinstaller.common.InstallerNodeFactory;
import com.fsoinstaller.common.InstallerNodeParseException;
import com.fsoinstaller.common.InstallerNode.InstallUnit;
import com.fsoinstaller.common.InstallerNode.RenamePair;
import com.fsoinstaller.internet.Connector;
import com.fsoinstaller.internet.DownloadEvent;
import com.fsoinstaller.internet.DownloadListener;
import com.fsoinstaller.internet.Downloader;
import com.fsoinstaller.main.Configuration;
import com.fsoinstaller.main.FreeSpaceOpenInstaller;
import com.fsoinstaller.utils.CollapsiblePanel;
import com.fsoinstaller.utils.Logger;
import com.fsoinstaller.utils.MiscUtils;

import static com.fsoinstaller.wizard.GUIConstants.*;


public class InstallItem extends JPanel
{
	private static final Logger logger = Logger.getLogger(InstallItem.class);
	
	private final InstallerNode node;
	private final List<ChangeListener> listenerList;
	
	private final JProgressBar overallBar;
	private final JButton cancelButton;
	private final StoplightPanel stoplightPanel;
	
	public InstallItem(InstallerNode node)
	{
		super();
		this.node = node;
		this.listenerList = new CopyOnWriteArrayList<ChangeListener>();
		
		setBorder(BorderFactory.createEmptyBorder(SMALL_MARGIN, SMALL_MARGIN, SMALL_MARGIN, SMALL_MARGIN));
		setLayout(new BoxLayout(this, BoxLayout.Y_AXIS));
		
		overallBar = new JProgressBar(0, GUIConstants.BAR_MAXIMUM);
		overallBar.setIndeterminate(true);
		overallBar.setString("Installing...");
		overallBar.setStringPainted(true);
		
		cancelButton = new JButton("Cancel");
		
		stoplightPanel = new StoplightPanel((int) cancelButton.getPreferredSize().getHeight());
		
		JPanel progressPanel = new JPanel();
		progressPanel.setLayout(new BoxLayout(progressPanel, BoxLayout.X_AXIS));
		progressPanel.add(overallBar);
		progressPanel.add(Box.createHorizontalStrut(GUIConstants.SMALL_MARGIN));
		progressPanel.add(cancelButton);
		progressPanel.add(Box.createHorizontalStrut(GUIConstants.SMALL_MARGIN));
		progressPanel.add(stoplightPanel);
		
		JPanel headerPanel = new JPanel(new BorderLayout(0, GUIConstants.SMALL_MARGIN));
		headerPanel.add(new JLabel(node.getName()), BorderLayout.NORTH);
		headerPanel.add(progressPanel, BorderLayout.CENTER);
		
		JPanel downloadsPanel = new JPanel();
		downloadsPanel.setLayout(new BoxLayout(downloadsPanel, BoxLayout.Y_AXIS));
		downloadsPanel.add(new DownloadPanel());
		downloadsPanel.add(new DownloadPanel());
		downloadsPanel.add(new DownloadPanel());
		
		CollapsiblePanel panel = new CollapsiblePanel(headerPanel, downloadsPanel);
		panel.setCollapsed(true);
		
		add(panel);
		add(Box.createGlue());
		
		// TODO if an item is dependent on things, then queue the dependencies FIRST, then unqueue and re-queue the current task
		
	}
	
	public void addCompletionListener(ChangeListener listener)
	{
		listenerList.add(listener);
	}
	
	public void removeCompletionListener(ChangeListener listener)
	{
		listenerList.remove(listener);
	}
	
	protected void fireCompletion()
	{
		ChangeEvent event = null;
		
		for (ChangeListener listener: listenerList)
		{
			if (event == null)
				event = new ChangeEvent(node);
			
			listener.stateChanged(event);
		}
	}
	
	public void start()
	{
		// TODO
	}
	
	/**
	 * Perform the installation tasks for this node.
	 * 
	 * @throws SecurityException if e.g. we are not allowed to create a folder
	 *         or download files to it
	 */
	public InstallItem call() throws SecurityException
	{
		File installDir = Configuration.getInstance().getApplicationDir();
		String nodeName = node.getName();
		
		logger.info(nodeName + ": Starting processing");
		setText("Setting up the mod...");
		
		// create the folder for this mod, if it has one
		File folder;
		String folderName = node.getFolder();
		if (folderName == null || folderName.length() == 0 || folderName.equals("/") || folderName.equals("\\"))
		{
			logger.debug(nodeName + ": This node has no folder; using application folder instead");
			folder = installDir;
		}
		else
		{
			logger.info(nodeName + ": Creating folder '" + folderName + "'");
			folder = new File(installDir, folderName);
			if (!folder.exists() && !folder.mkdir())
			{
				logger.error(nodeName + ": Unable to create the '" + folderName + "' folder!");
				return this;
			}
		}
		
		if (!node.getDeleteList().isEmpty())
		{
			logger.info(nodeName + ": Processing DELETE items");
			setText("Deleting old files...");
			
			// delete what we need to
			for (String delete: node.getDeleteList())
			{
				logger.debug(nodeName + ": Deleting '" + delete + "'");
				File file = new File(installDir, delete);
				if (file.exists())
				{
					if (file.isDirectory())
						logger.debug(nodeName + ": Cannot delete '" + delete + "'; deleting directories is not supported at this time");
					else if (!file.delete())
					{
						logger.error(nodeName + ": Unable to delete '" + delete + "'!");
						return this;
					}
				}
			}
		}
		
		if (!node.getRenameList().isEmpty())
		{
			logger.info(nodeName + ": Processing RENAME items");
			setText("Renaming files...");
			
			// rename what we need to
			for (RenamePair rename: node.getRenameList())
			{
				logger.debug(nodeName + ": Renaming '" + rename.getFrom() + "' to '" + rename.getTo() + "'");
				File from = new File(installDir, rename.getFrom());
				File to = new File(installDir, rename.getTo());
				if (!from.exists())
					logger.debug(nodeName + ": Cannot rename '" + rename.getFrom() + "'; it does not exist");
				else if (to.exists())
					logger.debug(nodeName + ": Cannot rename '" + rename.getFrom() + "' to '" + rename.getTo() + "'; the latter already exists");
				else if (!from.renameTo(to))
				{
					logger.error(nodeName + ": Unable to rename '" + rename.getFrom() + "' to '" + rename.getTo() + "'!");
					return this;
				}
			}
		}
		
		if (!node.getInstallList().isEmpty())
		{
			logger.info(nodeName + ": Processing INSTALL items");
			setText("Installing files...");
			
			Connector connector = (Connector) Configuration.getInstance().getSettings().get(Configuration.CONNECTOR_KEY);
			
			// these could be files to download, or they could later be files to extract
			for (InstallUnit install: node.getInstallList())
			{
				// TODO
				//todo();	// try mirrors in random order
				
				List<BaseURL> urls = install.getBaseURLList();
				for (String file: install.getFileList())
				{
					// attempt to install this file
					boolean succeeded = installOne(nodeName, connector, folder, urls, file);
					if (!succeeded)
						return this;
					logger.info(nodeName + ": Downloaded '" + file + "'");
				}
			}
		}
		
		// TODO: hash lists
		if (!node.getHashList().isEmpty())
		{
			logger.info(nodeName + ": Processing HASH items");
			setText("Computing hash values...");
		}
		
		setText("Done!");
		fireCompletion();
		return this;
	}
	
	private boolean installOne(String nodeName, Connector connector, File modFolder, List<BaseURL> baseURLList, String file)
	{
		logger.info(nodeName + ": installing '" + file + "'");
		
		// this will listen and update the bar until we have a download that succeeds
		BarUpdater updater = new BarUpdater(this, file);
		
		// try all URLs supplied
		for (BaseURL baseURL: baseURLList)
		{
			logger.debug(nodeName + ": Obtaining URL");
			URL url;
			try
			{
				url = baseURL.toURL(file);
			}
			catch (MalformedURLException murle)
			{
				logger.error(nodeName + ": Bad URL '" + baseURL.toString() + file + "'", murle);
				continue;
			}
			
			logger.debug(nodeName + ": Beginning download of '" + file + "'");
			Downloader downloader = new Downloader(connector, url, modFolder);
			downloader.addDownloadListener(updater);
			downloader.download();
			
			// did it work?
			if (updater.succeeded())
			{
				logger.debug(nodeName + ": Completed download of '" + file + "'");
				return true;
			}
		}
		
		logger.debug(nodeName + ": All mirror sites for '" + file + "' failed!");
		return false;
	}
	
	public void setIndeterminate(final boolean indeterminate)
	{
		EventQueue.invokeLater(new Runnable()
		{
			public void run()
			{
				overallBar.setIndeterminate(indeterminate);
			}
		});
	}
	
	public void setPercentComplete(int percent)
	{
		if (percent < 0)
			percent = 0;
		if (percent > 100)
			percent = 100;
		
		final int _percent = percent;
		EventQueue.invokeLater(new Runnable()
		{
			public void run()
			{
				overallBar.setValue(_percent);
			}
		});
	}
	
	public void setRatioComplete(double ratio)
	{
		setPercentComplete((int) (ratio * 100.0));
	}
	
	public void setText(final String text)
	{
		EventQueue.invokeLater(new Runnable()
		{
			public void run()
			{
				overallBar.setString(text);
			}
		});
	}
	
	private static class BarUpdater implements DownloadListener
	{
		private final InstallItem item;
		private final String file;
		private volatile boolean succeeded;
		
		public BarUpdater(InstallItem item, String file)
		{
			this.item = item;
			this.file = file;
			this.succeeded = false;
		}
		
		public boolean succeeded()
		{
			return succeeded;
		}
		
		public void downloadAboutToStart(DownloadEvent event)
		{
			item.setIndeterminate(false);
			item.setPercentComplete(0);
			item.setText("Downloading '" + file + "'...");
		}
		
		public void downloadComplete(DownloadEvent event)
		{
			item.setPercentComplete(100);
			item.setText("Downloading '" + file + "'...complete!");
			succeeded = true;
		}
		
		public void downloadFailed(DownloadEvent event)
		{
			item.setText("Downloading '" + file + "'...failed!");
			Exception e = event.getException();
			if (e != null)
				logger.error("Download of '" + event.getDownloadName() + "' failed!", e);
			succeeded = false;
		}
		
		public void downloadNotNecessary(DownloadEvent event)
		{
			item.setPercentComplete(100);
			item.setText("Downloading '" + file + "'...complete!");
			succeeded = true;
		}
		
		public void downloadProgressReport(DownloadEvent event)
		{
			item.setRatioComplete(((double) event.getDownloadedBytes()) / event.getTotalBytes());
			item.setText("Downloading '" + file + "'..." + event.getDownloadedBytes() + " of " + event.getTotalBytes() + " bytes");
		}
	}
	
	public static void main(String[] args) throws IOException, InstallerNodeParseException, ClassNotFoundException, InstantiationException, IllegalAccessException, UnsupportedLookAndFeelException
	{
		UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		
		Reader reader = new FileReader("fsport.txt");
		InstallerNode fsport = InstallerNodeFactory.readNode(reader);
		reader.close();
		
		// this little trick prevents the install items from stretching if there aren't enough to fill the vertical space
		JPanel scrollPanel = new JPanel(new BorderLayout());
		scrollPanel.add(new InstallItem(fsport), BorderLayout.NORTH);
		scrollPanel.add(Box.createGlue(), BorderLayout.CENTER);
		
		JScrollPane installScrollPane = new JScrollPane(scrollPanel, ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		
		JPanel panel = new JPanel(new BorderLayout(0, GUIConstants.DEFAULT_MARGIN));
		panel.setBorder(BorderFactory.createEmptyBorder(GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN));
		panel.add(installScrollPane, BorderLayout.CENTER);
		
		JFrame frame = new JFrame("test");
		frame.setContentPane(panel);
		frame.pack();
		frame.setDefaultCloseOperation(JFrame.DISPOSE_ON_CLOSE);
		
		MiscUtils.centerWindowOnScreen(frame);
		frame.setVisible(true);
		
		FreeSpaceOpenInstaller.getInstance().getExecutorService().shutdown();
	}
}
