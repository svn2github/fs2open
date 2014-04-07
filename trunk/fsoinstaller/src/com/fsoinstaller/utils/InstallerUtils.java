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

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import com.fsoinstaller.common.BaseURL;
import com.fsoinstaller.common.InstallerNode;
import com.fsoinstaller.common.InvalidBaseURLException;
import com.fsoinstaller.common.InstallerNode.FilePair;
import com.fsoinstaller.common.InstallerNode.HashTriple;
import com.fsoinstaller.common.InstallerNode.InstallUnit;
import com.fsoinstaller.main.Configuration;
import com.fsoinstaller.main.FreeSpaceOpenInstaller;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;


public class InstallerUtils
{
	private static final Logger logger = Logger.getLogger(InstallerUtils.class);
	
	private InstallerUtils()
	{
	}
	
	public static List<InstallerNode> maybeCreateAutomaticNodes()
	{
		Configuration configuration = Configuration.getInstance();
		List<InstallerNode> nodes = new ArrayList<InstallerNode>();
		
		// if OpenAL needs to be installed, do that first
		// (note, we can't use a primitive boolean because it may be null)
		Boolean installOpenAL = (Boolean) configuration.getSettings().get(Configuration.ADD_OPENAL_INSTALL_KEY);
		if (installOpenAL == Boolean.TRUE)
		{
			InstallerNode openAL = new InstallerNode(XSTR.getString("installOpenALName"));
			openAL.setDescription(XSTR.getString("installOpenALDesc"));
			openAL.setFolder(File.separator);
			openAL.setVersion(versionUUID());
			InstallUnit installUnit = new InstallUnit();
			for (String url: FreeSpaceOpenInstaller.INSTALLER_HOME_URLs)
			{
				try
				{
					installUnit.addBaseURL(new BaseURL(url));
				}
				catch (InvalidBaseURLException iburle)
				{
					logger.error("Impossible error: Internal home URLs should always be correct!", iburle);
				}
			}
			installUnit.addFile("oalinst.exe");
			openAL.addInstall(installUnit);
			
			openAL.addHashTriple(new HashTriple("MD5", "oalinst.exe", "694F54BD227916B89FC3EB1DB53F0685"));
			
			openAL.addExecCmd("oalinst.exe");
			
			openAL.setNote(XSTR.getString("openALNote"));
			
			if (!installUnit.getBaseURLList().isEmpty())
				nodes.add(openAL);
		}
		
		// all of the optional nodes, except for OpenAL, only apply for FS2
		if (configuration.requiresFS2())
		{
			// if GOG needs to be installed, do so
			File gogInstallPackage = (File) configuration.getSettings().get(Configuration.GOG_INSTALL_PACKAGE_KEY);
			InstallerNode gog = null;
			if (gogInstallPackage != null)
			{
				gog = new InstallerNode(XSTR.getString("installGOGName"));
				gog.setDescription(XSTR.getString("installGOGDesc"));
				gog.setFolder(File.separator);
				gog.setVersion(versionUUID());
				InstallUnit installUnit = new InstallUnit();
				for (String url: FreeSpaceOpenInstaller.INSTALLER_HOME_URLs)
				{
					try
					{
						installUnit.addBaseURL(new BaseURL(url));
					}
					catch (InvalidBaseURLException iburle)
					{
						logger.error("Impossible error: Internal home URLs should always be correct!", iburle);
					}
				}
				installUnit.addFile("innoextract.exe");
				gog.addInstall(installUnit);
				
				gog.addHashTriple(new HashTriple("MD5", "innoextract.exe", "98C89ADFAB259E4CC89D3CC18CEE2ADC"));
				
				StringBuilder cmd = new StringBuilder("innoextract -L -q --progress=true -e ");
				cmd.append(gogInstallPackage.getAbsolutePath());
				gog.addExecCmd(cmd.toString());
				
				if (!installUnit.getBaseURLList().isEmpty())
					nodes.add(gog);
			}
			
			// if version 1.2 patch needs to be applied, then add it
			String hash = (String) configuration.getSettings().get(Configuration.ROOT_FS2_VP_HASH_KEY);
			if (hash != null && hash.equalsIgnoreCase("42bc56a410373112dfddc7985f66524a"))
			{
				InstallerNode patchTo1_2 = new InstallerNode(XSTR.getString("installPatchName"));
				patchTo1_2.setDescription(XSTR.getString("installPatchDesc"));
				patchTo1_2.setFolder(File.separator);
				patchTo1_2.setVersion(versionUUID());
				InstallUnit installUnit = new InstallUnit();
				for (String url: FreeSpaceOpenInstaller.INSTALLER_HOME_URLs)
				{
					try
					{
						installUnit.addBaseURL(new BaseURL(url));
					}
					catch (InvalidBaseURLException iburle)
					{
						logger.error("Impossible error: Internal home URLs should always be correct!", iburle);
					}
				}
				installUnit.addFile("fs21x-12.7z");
				patchTo1_2.addInstall(installUnit);
				
				patchTo1_2.addDelete("FRED2.exe");
				patchTo1_2.addDelete("FreeSpace2.exe");
				patchTo1_2.addDelete("FS2.exe");
				patchTo1_2.addDelete("UpdateLauncher.exe");
				patchTo1_2.addDelete("readme.txt");
				patchTo1_2.addDelete("root_fs2.vp");
				
				patchTo1_2.addHashTriple(new HashTriple("MD5", "FRED2.exe", "9b8a3dfeb586de9584056f9e8fa28bb5"));
				patchTo1_2.addHashTriple(new HashTriple("MD5", "FreeSpace2.exe", "091f3f06f596a48ba4e3c42d05ec379f"));
				patchTo1_2.addHashTriple(new HashTriple("MD5", "FS2.exe", "2c8133768ebd99faba5c00dd03ebf9ea"));
				patchTo1_2.addHashTriple(new HashTriple("MD5", "UpdateLauncher.exe", "babe53dc03c83067a3336f0c888c4ac2"));
				patchTo1_2.addHashTriple(new HashTriple("MD5", "readme.txt", "b4df1711c324516e497ce90b66f45de9"));
				patchTo1_2.addHashTriple(new HashTriple("MD5", "root_fs2.vp", "0d9fd69acfe8b29d616377b057d2fc04"));
				
				if (!installUnit.getBaseURLList().isEmpty())
					nodes.add(patchTo1_2);
			}
			
			// if any of the MVE files exist in data2 and data3, but not in data/movies, copy them
			InstallerNode copyMVEs = new InstallerNode(XSTR.getString("installCopyCutscenesName"));
			copyMVEs.setDescription(XSTR.getString("installCopyCutscenesDesc"));
			copyMVEs.setFolder(File.separator);
			copyMVEs.setVersion(versionUUID());
			boolean doCopy = false;
			for (KeyPair<String, String> pair: Configuration.GOG_MOVIES)
			{
				// e.g. data2/INTRO.MVE
				String source = pair.getObject2() + File.separator + pair.getObject1();
				// e.g. data/movies/INTRO.MVE
				String dest = "data" + File.separator + "movies" + File.separator + pair.getObject1();
				
				// anticipate copying the files
				copyMVEs.addCopyPair(new FilePair(source, dest));
				
				// check whether at least one file needs to be copied
				if ((new File(configuration.getApplicationDir(), source)).exists() && !(new File(configuration.getApplicationDir(), dest)).exists())
					doCopy = true;
			}
			if (gog != null)
				gog.addChild(copyMVEs);
			else if (doCopy)
				nodes.add(copyMVEs);
		}
		
		return nodes;
	}
	
	private static String versionUUID()
	{
		return "UUID" + UUID.randomUUID().toString().replaceAll("-", "");
	}
}
