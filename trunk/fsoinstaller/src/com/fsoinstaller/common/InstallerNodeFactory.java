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

import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Pattern;

import com.fsoinstaller.utils.IOUtils;
import com.fsoinstaller.utils.Logger;
import com.fsoinstaller.utils.MiscUtils;


/**
 * Mostly for parsing and printing.
 * 
 * @author Goober5000
 */
public class InstallerNodeFactory
{
	private static final Logger logger = Logger.getLogger(InstallerNodeFactory.class);
	
	private static final Pattern SPACE_OR_TAB_PATTERN = Pattern.compile("[ \t]");
	static final Pattern TOKEN_PATTERN = Pattern.compile("[A-Z_]+");
	
	public static InstallerNode readNode(Reader reader) throws InstallerNodeParseException, IOException
	{
		// skip strings until NAME or EOF is reached
		Object object;
		while (true)
		{
			object = readStringOrToken(reader);
			if (object == null)
				return null;
			else if (object == InstallerNodeToken.NAME)
				break;
			else if (object instanceof InstallerNodeToken)
				throw new InstallerNodeParseException("Unexpected token '" + object + "' found while looking for '" + InstallerNodeToken.NAME + "'!");
		}
		
		return readNodeSub(reader);
	}
	
	private static InstallerNode readNodeSub(Reader reader) throws InstallerNodeParseException, IOException
	{
		// create the node with its name
		InstallerNode node = new InstallerNode(readString(reader));
		
		// populate this node
		Object object = null;
		InstallerNode.InstallUnit currentInstallUnit = null;
		while (true)
		{
			object = readStringOrToken(reader);
			if (object == null)
				throw new InstallerNodeParseException("End of reader reached before parsing completed!");
			
			// child node
			if (object == InstallerNodeToken.NAME)
			{
				node.addChild(readNodeSub(reader));
			}
			// end of a node
			else if (object == InstallerNodeToken.END)
			{
				break;
			}
			// handle a token
			else if (object instanceof InstallerNodeToken)
			{
				// each separate URL/string combo requires a new install unit
				if (object == InstallerNodeToken.URL || object == InstallerNodeToken.MULTIURL)
				{
					currentInstallUnit = new InstallerNode.InstallUnit();
					node.addInstall(currentInstallUnit);
				}
				// any other token resets it
				else
				{
					currentInstallUnit = null;
				}
				
				handleToken(reader, (InstallerNodeToken) object, node, currentInstallUnit);
			}
			// we have an unannotated string
			else if (((String) object).length() != 0)
			{
				String string = (String) object;
				
				// if it's all-caps and has no spaces, it's highly likely to be a token from the future
				if (TOKEN_PATTERN.matcher(string).matches())
				{
					logger.debug("Skipping possible token from a future version: " + string);
					
					// clear the install unit and do nothing else, causing us to ignore it until the next recognized token
					currentInstallUnit = null;
				}
				// not in token format, but still unannotated... probably input for a future token
				else if (currentInstallUnit == null)
				{
					logger.debug("Skipping string: " + string);
				}
				// an active install unit means we install this string
				else
				{
					currentInstallUnit.addFile((String) object);
				}
			}
		}
		
		return node;
	}
	
	private static void handleToken(Reader reader, InstallerNodeToken token, InstallerNode node, InstallerNode.InstallUnit currentInstallUnit) throws InstallerNodeParseException, IOException
	{
		switch (token)
		{
			case DESC:
				String desc = readStringUntilEndToken(reader, InstallerNodeToken.ENDDESC);
				node.setDescription(desc);
				break;
			
			case FOLDER:
				String folder = MiscUtils.standardizeSlashes(readString(reader));
				node.setFolder(folder);
				break;
			
			case DELETE:
				String delete = MiscUtils.standardizeSlashes(readString(reader));
				node.addDelete(delete);
				break;
			
			case RENAME:
				String renameFrom = MiscUtils.standardizeSlashes(readString(reader));
				String renameTo = MiscUtils.standardizeSlashes(readString(reader));
				node.addRenamePair(new InstallerNode.FilePair(renameFrom, renameTo));
				break;
			
			case COPY:
				String copyFrom = MiscUtils.standardizeSlashes(readString(reader));
				String copyTo = MiscUtils.standardizeSlashes(readString(reader));
				node.addCopyPair(new InstallerNode.FilePair(copyFrom, copyTo));
				break;
			
			case URL:
				String url = readString(reader);
				try
				{
					currentInstallUnit.addBaseURL(new BaseURL(url));
				}
				catch (InvalidBaseURLException ibue)
				{
					throw new InstallerNodeParseException(ibue.getMessage(), ibue);
				}
				break;
			
			case MULTIURL:
				List<String> multiURLList = readStringsUntilEndToken(reader, InstallerNodeToken.ENDMULTI);
				try
				{
					for (String multiURL: multiURLList)
						currentInstallUnit.addBaseURL(new BaseURL(multiURL));
				}
				catch (InvalidBaseURLException ibue)
				{
					throw new InstallerNodeParseException(ibue.getMessage(), ibue);
				}
				break;
			
			case HASH:
				String line = readString(reader);
				String type, filename, hash;
				
				// could be all on one line or on three lines
				String[] parts = SPACE_OR_TAB_PATTERN.split(line);
				if (parts.length == 3)
				{
					type = parts[0];
					filename = MiscUtils.standardizeSlashes(parts[1]);
					hash = parts[2];
				}
				else
				{
					type = line;
					filename = MiscUtils.standardizeSlashes(readString(reader));
					hash = readString(reader);
				}
				
				node.addHashTriple(new InstallerNode.HashTriple(type, filename, hash));
				break;
			
			case NOTE:
				String note = readStringUntilEndToken(reader, InstallerNodeToken.ENDNOTE);
				node.setNote(note);
				break;
			
			case DEPENDENCIES:
				List<String> dependencies = readStringsUntilEndToken(reader, InstallerNodeToken.ENDDEPENDENCIES);
				for (String dependency: dependencies)
					node.addDependency(dependency);
				break;
			
			case VERSION:
				String version = readString(reader);
				node.setVersion(version);
				break;
			
			case ENDDESC:
			case ENDMULTI:
			case ENDNOTE:
				throw new InstallerNodeParseException("Unexpected token '" + token + "' found!");
				
			case NAME:
			case END:
				throw new Error("The token '" + token + "' should have been handled already!");
				
			default:
				throw new Error("Unhandled token '" + token + "'!");
		}
	}
	
	private static String readString(Reader reader) throws InstallerNodeParseException, IOException
	{
		Object object = readStringOrToken(reader);
		if (object == null)
			throw new InstallerNodeParseException("End of reader reached before parsing completed!");
		else if (object instanceof InstallerNodeToken)
			throw new InstallerNodeParseException("Expected a plain string; found the token '" + object + "'!");
		
		return (String) object;
	}
	
	private static List<String> readStringsUntilEndToken(Reader reader, InstallerNodeToken endToken) throws InstallerNodeParseException, IOException
	{
		List<String> strings = new ArrayList<String>();
		
		while (true)
		{
			Object object = readStringOrToken(reader);
			if (object == null)
				throw new InstallerNodeParseException("End of reader reached before parsing completed!");
			else if (object == endToken)
				break;
			else if (object instanceof InstallerNodeToken)
				throw new InstallerNodeParseException("Unexpected token '" + object + "' found while looking for '" + endToken + "'!");
			
			strings.add((String) object);
		}
		
		return strings;
	}
	
	private static String readStringUntilEndToken(Reader reader, InstallerNodeToken endToken) throws InstallerNodeParseException, IOException
	{
		List<String> strings = readStringsUntilEndToken(reader, endToken);
		Iterator<String> ii = strings.iterator();
		if (!ii.hasNext())
			return "";
		
		StringBuilder builder = new StringBuilder(ii.next());
		while (ii.hasNext())
		{
			builder.append('\n');
			builder.append(ii.next());
		}
		return builder.toString();
	}
	
	private static Object readStringOrToken(Reader reader) throws IOException
	{
		String line = readLine(reader);
		if (line == null)
			return null;
		
		for (InstallerNodeToken token: InstallerNodeToken.values())
		{
			if (token.getToken().equals(line))
				return token;
		}
		
		return line;
	}
	
	/**
	 * Returns the next line of input, trimmed of whitespace on either side.
	 * Returns null if the end of the reader has been reached.
	 */
	private static String readLine(Reader reader) throws IOException
	{
		StringBuilder builder = new StringBuilder();
		while (true)
		{
			int ch = reader.read();
			if (ch < 0)
				return builder.length() > 0 ? builder.toString().trim() : null;
			else if (ch == '\n')
				break;
			builder.append((char) ch);
		}
		return builder.toString().trim();
	}
	
	public static void writeNode(Writer writer, InstallerNode node) throws IOException
	{
		writeNode(0, writer, node);
	}
	
	public static void writeNode(int indent, Writer writer, InstallerNode node) throws IOException
	{
		writeLine(indent, writer, InstallerNodeToken.NAME, node.getName());
		
		if (node.getDescription() != null)
			writeLine(indent, writer, InstallerNodeToken.DESC, node.getDescription(), InstallerNodeToken.ENDDESC);
		
		if (node.getFolder() != null)
			writeLine(indent, writer, InstallerNodeToken.FOLDER, node.getFolder());
		
		for (String delete: node.getDeleteList())
			writeLine(indent, writer, InstallerNodeToken.DELETE, delete);
		
		for (InstallerNode.FilePair pair: node.getRenameList())
			writeLine(indent, writer, InstallerNodeToken.RENAME, pair.getFrom(), pair.getTo());
		
		for (InstallerNode.FilePair pair: node.getCopyList())
			writeLine(indent, writer, InstallerNodeToken.COPY, pair.getFrom(), pair.getTo());
		
		for (InstallerNode.InstallUnit unit: node.getInstallList())
		{
			// first URLs
			if (unit.getBaseURLList().size() == 1)
				writeLine(indent, writer, InstallerNodeToken.URL, unit.getBaseURLList().get(0).toString());
			else if (unit.getBaseURLList().size() > 1)
			{
				writeLine(indent, writer, InstallerNodeToken.MULTIURL);
				for (BaseURL baseURL: unit.getBaseURLList())
					writeLine(indent, writer, baseURL.toString());
				writeLine(indent, writer, InstallerNodeToken.ENDMULTI);
			}
			
			// then install items
			for (String install: unit.getFileList())
				writeLine(indent, writer, install);
		}
		
		for (InstallerNode.HashTriple triple: node.getHashList())
			writeLine(indent, writer, InstallerNodeToken.HASH, triple.getType(), triple.getFilename(), triple.getHash());
		
		if (!node.getChildren().isEmpty())
		{
			writeLine(indent, writer, "");
			for (InstallerNode child: node.getChildren())
				writeNode(indent + 1, writer, child);
		}
		
		if (node.getNote() != null)
			writeLine(indent, writer, InstallerNodeToken.NOTE, node.getNote(), InstallerNodeToken.ENDNOTE);
		
		if (!node.getDependencyList().isEmpty())
		{
			writeLine(indent, writer, InstallerNodeToken.DEPENDENCIES);
			for (String dependency: node.getDependencyList())
				writeLine(indent, writer, dependency);
			writeLine(indent, writer, InstallerNodeToken.ENDDEPENDENCIES);
		}
		
		if (node.getVersion() != null)
			writeLine(indent, writer, InstallerNodeToken.VERSION, node.getVersion());
		
		writeLine(indent, writer, InstallerNodeToken.END);
		writeLine(indent, writer, "");
	}
	
	private static void writeLine(int indent, Writer writer, Object ... objects) throws IOException
	{
		for (Object object: objects)
		{
			if (object instanceof String)
				writeLine(indent, writer, (String) object);
			else if (object instanceof InstallerNodeToken)
				writeLine(indent, writer, ((InstallerNodeToken) object).getToken());
			else
				writeLine(indent, writer, object.toString());
		}
	}
	
	private static void writeLine(int indent, Writer writer, String string) throws IOException
	{
		String trimmed = string.trim();
		if (trimmed.length() != 0)
		{
			for (int i = 0; i < indent; i++)
				writer.write('\t');
			writer.write(trimmed);
		}
		writer.write(IOUtils.ENDL);
	}
}
