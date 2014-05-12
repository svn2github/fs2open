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

package com.fsoinstaller.common;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;


/**
 * This class represents a single unit of installation, such as a mod, with a
 * group of associated files.
 * 
 * @author Goober5000
 */
public class InstallerNode
{
	protected String name;
	protected String description;
	protected String folder;
	protected String version;
	protected String note;
	
	protected String treePath;
	
	protected final List<String> deleteList;
	protected final List<FilePair> renameList;
	protected final List<FilePair> copyList;
	protected final List<InstallUnit> installList;
	protected final List<HashTriple> hashList;
	protected final List<String> cmdList;
	
	protected InstallerNode parent;
	protected final List<InstallerNode> children;
	
	protected Object userObject;
	
	public InstallerNode(String name)
	{
		if (name == null)
			throw new NullPointerException("The 'name' field cannot be null!");
		
		this.name = name;
		this.description = null;
		this.folder = null;
		this.version = null;
		this.note = null;
		
		this.deleteList = new ArrayList<String>();
		this.renameList = new ArrayList<FilePair>();
		this.copyList = new ArrayList<FilePair>();
		this.installList = new ArrayList<InstallUnit>();
		this.hashList = new ArrayList<HashTriple>();
		this.cmdList = new ArrayList<String>();
		
		this.parent = null;
		this.children = new ArrayList<InstallerNode>();
		
		this.userObject = null;
	}
	
	public String getName()
	{
		return name;
	}
	
	public void setName(String name)
	{
		if (name == null)
			throw new NullPointerException("The 'name' field cannot be null!");
		
		this.name = name;
	}
	
	/**
	 * It is assumed that the tree path of a node will never change after a
	 * certain point before which we care about calling this method.
	 * <p>
	 * (This isn't synchronized, but since the tree path is assumed to be
	 * immutable, that shouldn't matter.)
	 */
	public String getTreePath()
	{
		if (treePath == null)
		{
			// construct tree path to this node
			StringBuilder builder = new StringBuilder();
			InstallerNode ii = this;
			while (ii != null)
			{
				builder.insert(0, '.');
				builder.insert(0, ii.getName());
				ii = ii.getParent();
			}
			
			// get rid of trailing period
			builder.setLength(builder.length() - 1);
			
			treePath = builder.toString();
		}
		
		return treePath;
	}
	
	public String getDescription()
	{
		return description;
	}
	
	public void setDescription(String description)
	{
		this.description = description;
	}
	
	public String getFolder()
	{
		return folder;
	}
	
	public void setFolder(String folder)
	{
		this.folder = folder;
	}
	
	public String getVersion()
	{
		return version;
	}
	
	public void setVersion(String version)
	{
		this.version = version;
	}
	
	public String getNote()
	{
		return note;
	}
	
	public void setNote(String note)
	{
		this.note = note;
	}
	
	public List<String> getDeleteList()
	{
		return Collections.unmodifiableList(deleteList);
	}
	
	public List<FilePair> getRenameList()
	{
		return Collections.unmodifiableList(renameList);
	}
	
	public List<FilePair> getCopyList()
	{
		return Collections.unmodifiableList(copyList);
	}
	
	public List<InstallUnit> getInstallList()
	{
		return Collections.unmodifiableList(installList);
	}
	
	public List<HashTriple> getHashList()
	{
		return Collections.unmodifiableList(hashList);
	}
	
	public List<String> getExecCmdList()
	{
		return Collections.unmodifiableList(cmdList);
	}
	
	public List<InstallerNode> getChildren()
	{
		return Collections.unmodifiableList(children);
	}
	
	public InstallerNode getParent()
	{
		return parent;
	}
	
	public Object getUserObject()
	{
		return userObject;
	}
	
	public void addDelete(String deleteItem)
	{
		if (deleteItem == null)
			throw new NullPointerException("Cannot add a null delete item!");
		
		deleteList.add(deleteItem);
	}
	
	public void removeDelete(String deleteItem)
	{
		deleteList.remove(deleteItem);
	}
	
	public void addRenamePair(FilePair renamePair)
	{
		if (renamePair == null)
			throw new NullPointerException("Cannot add a null rename pair!");
		
		renameList.add(renamePair);
	}
	
	public void removeRenamePair(FilePair renamePair)
	{
		renameList.remove(renamePair);
	}
	
	public void addCopyPair(FilePair copyPair)
	{
		if (copyPair == null)
			throw new NullPointerException("Cannot add a null copy pair!");
		
		copyList.add(copyPair);
	}
	
	public void removeCopyPair(FilePair copyPair)
	{
		copyList.remove(copyPair);
	}
	
	public void addInstall(InstallUnit installUnit)
	{
		if (installUnit == null)
			throw new NullPointerException("Cannot add a null install unit!");
		
		installList.add(installUnit);
	}
	
	public void removeInstall(InstallUnit installUnit)
	{
		installList.remove(installUnit);
	}
	
	public void addHashTriple(HashTriple hashTriple)
	{
		if (hashTriple == null)
			throw new NullPointerException("Cannot add a null hash triple!");
		
		hashList.add(hashTriple);
	}
	
	public void removeHashTriple(HashTriple hashTriple)
	{
		hashList.remove(hashTriple);
	}
	
	public void addExecCmd(String cmd)
	{
		if (cmd == null)
			throw new NullPointerException("Cannot add a null exec command!");
		
		cmdList.add(cmd);
	}
	
	public void removeExecCmd(String cmd)
	{
		cmdList.remove(cmd);
	}
	
	public void addChild(InstallerNode installerNode)
	{
		if (installerNode == null)
			throw new NullPointerException("Cannot add a null child!");
		
		children.add(installerNode);
		installerNode.parent = this;
	}
	
	public void removeChild(InstallerNode installerNode)
	{
		children.remove(installerNode);
		installerNode.parent = null;
	}
	
	public void setUserObject(Object userObject)
	{
		this.userObject = userObject;
	}
	
	@Override
	public boolean equals(Object object)
	{
		if (this == object)
			return true;
		if (object == null)
			return false;
		if (getClass() != object.getClass())
			return false;
		InstallerNode other = (InstallerNode) object;
		return name.equals(other.name);
	}
	
	@Override
	public int hashCode()
	{
		return name.hashCode();
	}
	
	@Override
	public String toString()
	{
		return name;
	}
	
	public InstallerNode findInTree(String name)
	{
		if (name == null)
			throw new NullPointerException("Name cannot be null!");
		
		if (this.name.equals(name))
			return this;
		
		for (InstallerNode child: this.children)
		{
			InstallerNode result = child.findInTree(name);
			if (result != null)
				return result;
		}
		
		return null;
	}
	
	public static class FilePair
	{
		private String from;
		private String to;
		
		public FilePair(String from, String to)
		{
			if (from == null || to == null)
				throw new NullPointerException("Arguments cannot be null!");
			
			this.from = from;
			this.to = to;
		}
		
		public String getFrom()
		{
			return from;
		}
		
		public void setFrom(String from)
		{
			if (from == null)
				throw new NullPointerException("The 'from' field cannot be null!");
			
			this.from = from;
		}
		
		public String getTo()
		{
			return to;
		}
		
		public void setTo(String to)
		{
			if (from == null)
				throw new NullPointerException("The 'to' field cannot be null!");
			
			this.to = to;
		}
	}
	
	public static class HashTriple
	{
		private String type;
		private String filename;
		private String hash;
		
		public HashTriple(String type, String filename, String hash)
		{
			if (type == null || filename == null || hash == null)
				throw new NullPointerException("Arguments cannot be null!");
			
			this.type = type;
			this.filename = filename;
			this.hash = hash;
		}
		
		public String getType()
		{
			return type;
		}
		
		public void setType(String type)
		{
			if (type == null)
				throw new NullPointerException("The 'type' field cannot be null!");
			
			this.type = type;
		}
		
		public String getFilename()
		{
			return filename;
		}
		
		public void setFilename(String filename)
		{
			if (filename == null)
				throw new NullPointerException("The 'filename' field cannot be null!");
			
			this.filename = filename;
		}
		
		public String getHash()
		{
			return hash;
		}
		
		public void setHash(String hash)
		{
			if (hash == null)
				throw new NullPointerException("The 'hash' field cannot be null!");
			
			this.hash = hash;
		}
	}
	
	public static class InstallUnit
	{
		private List<BaseURL> baseURLList;
		private List<String> fileList;
		
		public InstallUnit()
		{
			this.baseURLList = new ArrayList<BaseURL>();
			this.fileList = new ArrayList<String>();
		}
		
		public List<BaseURL> getBaseURLList()
		{
			return Collections.unmodifiableList(baseURLList);
		}
		
		public List<String> getFileList()
		{
			return Collections.unmodifiableList(fileList);
		}
		
		public void addBaseURL(BaseURL baseURL)
		{
			if (baseURL == null)
				throw new NullPointerException("Cannot add a null base URL!");
			
			// ensure no duplicates
			if (!baseURLList.contains(baseURL))
				baseURLList.add(baseURL);
		}
		
		public void removeBaseURL(BaseURL baseURL)
		{
			baseURLList.remove(baseURL);
		}
		
		public void addFile(String file)
		{
			if (file == null)
				throw new NullPointerException("Cannot add a null file!");
			
			// ensure no duplicates
			if (!fileList.contains(file))
				fileList.add(file);
		}
		
		public void removeFile(String file)
		{
			fileList.remove(file);
		}
	}
}
