/*
 * This file is part of the FreeSpace Open Installer
 * Copyright (C) 2014 The FreeSpace 2 Source Code Project
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

import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.LinkedList;
import java.util.List;
import java.util.concurrent.Callable;

import com.fsoinstaller.common.PayloadEvent;
import com.fsoinstaller.common.PayloadListener;
import com.fsoinstaller.main.Configuration;
import com.fsoinstaller.utils.IOUtils;
import com.fsoinstaller.utils.Logger;
import com.fsoinstaller.utils.MiscUtils;
import com.fsoinstaller.utils.MiscUtils.OperatingSystem;
import com.fsoinstaller.utils.ReaderLister;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;


/**
 * In parallel with the other Task classes in this directory, this encapsulates
 * out a bunch of stuff that would otherwise go in InstallItem. It runs
 * InnoExtract to copy all the FS2 program files from the GOG package to the
 * installation directory.
 * 
 * @author Goober5000
 */
class InnoExtractTask implements Callable<Boolean>
{
	private static final Logger logger = Logger.getLogger(InnoExtractTask.class);
	
	private final InstallItem item;
	private final File gogInstallPackage;
	
	public InnoExtractTask(InstallItem item)
	{
		// for the progress bar and so forth
		this.item = item;
		
		this.gogInstallPackage = (File) Configuration.getInstance().getSettings().get(Configuration.GOG_INSTALL_PACKAGE_KEY);
	}
	
	public Boolean call() throws InterruptedException
	{
		if (!gogInstallPackage.exists())
			throw new IllegalStateException("The GOG install package must exist!");
		
		// some operating systems are not supported
		OperatingSystem os = MiscUtils.determineOS();
		switch (os)
		{
			case WINDOWS:
				String os_name_lower = System.getProperty("os.name").toLowerCase();
				if (os_name_lower.contains("windows 95") || os_name_lower.contains("windows 98") || os_name_lower.contains("windows me"))
				{
					item.logInstallError(XSTR.getString("innoExtractRequiresXP"));
					return false;
				}
				// other versions of Windows are okay
				break;
			
			// we'll assume all versions of Linux/Unix are okay
			case UNIX:
				break;
			
			// no other OS is supported
			default:
				item.logInstallError(XSTR.getString("innoExtractNotSupported"));
				return false;
		}
		
		// the mod folder will also be the extraction destination
		File installDir = Configuration.getInstance().getApplicationDir();
		File extractDir = new File(installDir, item.getInstallerNode().getFolder());
		
		// bit of a hack... the first hash triple is the thing we run
		File innoExtractExecutable = new File(extractDir, item.getInstallerNode().getHashList().get(0).getFilename());
		
		// the first thing we do is obtain a list of the files that need to be extracted
		item.setIndeterminate(true);
		item.setText(XSTR.getString("innoExtractCountingFiles"));
		List<String> filesToBeExtracted;
		try
		{
			filesToBeExtracted = innoExtractListFiles(innoExtractExecutable);
		}
		catch (InterruptedException ie)
		{
			logger.error("Thread was interrupted while waiting for innoextract to complete!", ie);
			Thread.currentThread().interrupt();
			return false;
		}
		catch (IOException ioe)
		{
			logger.error("Could not obtain file listing using innoextract!", ioe);
			item.logInstallError(XSTR.getString("innoExtractCountingFilesFailed"));
			return false;
		}
		
		// now that we have the files, run the installation and report progress
		item.setIndeterminate(false);
		item.setText(XSTR.getString("innoExtractExtractingFiles"));
		try
		{
			innoExtractExtractFiles(innoExtractExecutable, extractDir, filesToBeExtracted.size());
		}
		catch (InterruptedException ie)
		{
			logger.error("Thread was interrupted while waiting for innoextract to complete!", ie);
			Thread.currentThread().interrupt();
			return false;
		}
		catch (IOException ioe)
		{
			logger.error("Could not extract files using innoextract!", ioe);
			item.logInstallError(XSTR.getString("innoExtractExtractingFilesFailed"));
			return false;
		}
		
		// now move all the files to their proper place
		item.setText(XSTR.getString("innoExtractMovingFiles"));
		if (!moveAppFiles(new File(extractDir, "app"), installDir))
		{
			logger.error("Could not move app files to the correct location!");
			item.logInstallError(XSTR.getString("innoExtractMovingFilesFailed"));
			return false;
		}
		
		// delete the folders that we no longer need
		item.setIndeterminate(true);
		item.setText(XSTR.getString("innoExtractDeletingTempFiles"));
		boolean result = IOUtils.deleteDirectoryTree(extractDir);
		item.setIndeterminate(false);
		if (!result)
		{
			logger.error("Could not delete the temporary directory tree!");
			item.logInstallError(String.format(XSTR.getString("innoExtractDeletingTempFilesFailed"), extractDir.getAbsolutePath()));
			// don't return false here because it's still playable
		}
		
		// we are done!
		return true;
	}
	
	private void runProcess(Process process, ReaderLister stdout, ReaderLister stderr) throws InterruptedException, IOException
	{
		// start the readers
		Thread t1 = new Thread(stdout, "InnoExtract-stdout");
		Thread t2 = new Thread(stderr, "InnoExtract-stderr");
		t1.setPriority(Thread.NORM_PRIORITY);
		t2.setPriority(Thread.NORM_PRIORITY);
		t1.start();
		t2.start();
		
		// block until the process completes
		int exitCode = process.waitFor();
		
		// get lists
		List<String> outlist = stdout.getList();
		List<String> errlist = stderr.getList();
		
		// report any error encountered
		if (exitCode != 0 || !errlist.isEmpty())
		{
			for (String line: outlist)
				logger.error("stdout: " + line);
			for (String line: errlist)
				logger.error("stderr: " + line);
			throw new IOException("InnoExtract reported exit code " + exitCode);
		}
	}
	
	private List<String> innoExtractListFiles(File innoExtractExecutable) throws InterruptedException, IOException
	{
		logger.info("Counting the number of files to be extracted...");
		
		// put together the args to list the files
		List<String> commands = new ArrayList<String>();
		commands.add(innoExtractExecutable.getName());
		commands.add("--quiet");
		commands.add("--list");
		commands.add(gogInstallPackage.getAbsolutePath());
		
		// we do things this way because InnoExtract does not need administrator privileges to run, and by skipping the shell we can receive the exit code of the process
		ProcessBuilder builder = new ProcessBuilder(commands);
		builder.directory(innoExtractExecutable.getParentFile());
		Process process = builder.start();
		
		// get the output
		ReaderLister stdout = new ReaderLister(new InputStreamReader(process.getInputStream()));
		ReaderLister stderr = new ReaderLister(new InputStreamReader(process.getErrorStream()));
		
		// take care of boilerplate stuff
		runProcess(process, stdout, stderr);
		
		// obtaining the listing was successful!
		logger.info("...done");
		return stdout.getList();
	}
	
	private void innoExtractExtractFiles(File innoExtractExecutable, File extractDir, final int totalFiles) throws InterruptedException, IOException
	{
		logger.info("Extracting " + totalFiles + " files...");
		
		// put together the args to extract the files
		List<String> commands = new ArrayList<String>();
		commands.add(innoExtractExecutable.getName());
		commands.add("--quiet");
		commands.add("--output-dir");
		commands.add(extractDir.getAbsolutePath());
		commands.add(gogInstallPackage.getAbsolutePath());
		
		// we do things this way because InnoExtract does not need administrator privileges to run, and by skipping the shell we can receive the exit code of the process
		ProcessBuilder builder = new ProcessBuilder(commands);
		builder.directory(innoExtractExecutable.getParentFile());
		Process process = builder.start();
		
		// get the output
		ReaderLister stdout = new ReaderLister(new InputStreamReader(process.getInputStream()));
		ReaderLister stderr = new ReaderLister(new InputStreamReader(process.getErrorStream()));
		
		// update progress as the extraction proceeds
		item.setPercentComplete(0);
		stdout.addPayloadListener(new PayloadListener()
		{
			private int numFiles = 0;
			
			public void payloadReceived(PayloadEvent event)
			{
				numFiles++;
				item.setPercentComplete(numFiles * 100 / totalFiles);
				logger.debug(event.getPayload());
			}
		});
		
		// take care of boilerplate stuff
		runProcess(process, stdout, stderr);
		
		// extraction was successful!
		logger.info("...done");
	}
	
	private boolean moveAppFiles(File appFile, File installDir)
	{
		// if this is a directory, we iterate over it
		if (appFile.isDirectory())
		{
			File[] files = appFile.listFiles();
			if (files == null)
			{
				logger.error("There was a problem iterating over the directory '" + appFile.getAbsolutePath() + "'!");
				return false;
			}
			
			for (File file: files)
			{
				if (!moveAppFiles(file, installDir))
					return false;
			}
		}
		// if this is a file, we move it
		else
		{
			// build all the file parts until we get up to "app"
			LinkedList<String> parts = new LinkedList<String>();
			File temp = appFile;
			while (true)
			{
				String name = temp.getName();
				
				// root_fs2 shouldn't be capitalized
				if (name.equalsIgnoreCase("root_fs2.vp"))
					name = name.toLowerCase();
				// stop at "app"
				else if (name.equals("app"))
					break;
				
				parts.addFirst(name);
				
				// go up through the file tree
				temp = temp.getParentFile();
				if (temp == null)
					throw new IllegalStateException(appFile.getAbsolutePath() + " isn't part of the 'app' file tree!");
			}
			
			// now reconstruct the same path, but under the install dir instead
			File installFile = installDir;
			for (String part: parts)
				installFile = new File(installFile, part);
			
			// no need to move the file if the destination exists
			if (!installFile.exists())
			{
				// make sure the new directory structure exists
				if (!installFile.getParentFile().exists() && !installFile.getParentFile().mkdirs())
				{
					logger.error("Unable to rename '" + appFile.getAbsolutePath() + "' to '" + installFile.getAbsolutePath() + "': destination file tree could not be created!");
					return false;
				}
				
				// try to rename the files
				if (!appFile.renameTo(installFile))
				{
					logger.error("Unable to rename '" + appFile.getAbsolutePath() + "' to '" + installFile.getAbsolutePath() + "'!");
					return false;
				}
			}
		}
		
		return true;
	}
}
