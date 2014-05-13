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
import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.io.File;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import javax.swing.AbstractAction;
import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextPane;
import javax.swing.ScrollPaneConstants;

import com.fsoinstaller.common.InstallerNode;
import com.fsoinstaller.main.Configuration;
import com.fsoinstaller.utils.HSLColor;
import com.fsoinstaller.utils.IOUtils;
import com.fsoinstaller.utils.InstallerUtils;
import com.fsoinstaller.utils.Logger;
import com.fsoinstaller.utils.MiscUtils;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;


public class ModSelectPage extends WizardPage
{
	private static final Logger logger = Logger.getLogger(ModSelectPage.class);
	
	private final JPanel modPanel;
	private final JScrollPane modScrollPane;
	private final JCheckBox reRunCheckBox;
	
	private List<InstallerNode> modNodeTreeWalk;
	private List<InstallerNode> automaticNodeTreeWalk;
	
	private boolean inited;
	private SharedCounter counter;
	
	public ModSelectPage()
	{
		super("mod-select");
		
		// create widgets
		modPanel = new JPanel();
		modPanel.setLayout(new BoxLayout(modPanel, BoxLayout.Y_AXIS));
		modScrollPane = new JScrollPane(modPanel, ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		reRunCheckBox = new JCheckBox(XSTR.getString("modSelectPageCheckbox"));
		
		modNodeTreeWalk = null;
		automaticNodeTreeWalk = null;
		inited = false;
		counter = null;
	}
	
	@Override
	public JPanel createCenterPanel()
	{
		JPanel labelPanel = new JPanel();
		labelPanel.setLayout(new BoxLayout(labelPanel, BoxLayout.X_AXIS));
		labelPanel.add(new JLabel(XSTR.getString("modSelectPageText")));
		labelPanel.add(Box.createHorizontalGlue());
		
		JPanel checkBoxPanel = new JPanel();
		checkBoxPanel.setLayout(new BoxLayout(checkBoxPanel, BoxLayout.X_AXIS));
		checkBoxPanel.add(reRunCheckBox);
		checkBoxPanel.add(Box.createHorizontalGlue());
		
		labelPanel.setBorder(BorderFactory.createEmptyBorder(0, 0, GUIConstants.DEFAULT_MARGIN, 0));
		checkBoxPanel.setBorder(BorderFactory.createEmptyBorder(GUIConstants.SMALL_MARGIN, 0, GUIConstants.SMALL_MARGIN, 0));
		
		JPanel panel = new JPanel(new BorderLayout());
		panel.setBorder(BorderFactory.createEmptyBorder(GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN, 0, GUIConstants.DEFAULT_MARGIN));
		panel.add(labelPanel, BorderLayout.NORTH);
		panel.add(modScrollPane, BorderLayout.CENTER);
		panel.add(checkBoxPanel, BorderLayout.SOUTH);
		
		return panel;
	}
	
	@Override
	public void prepareForDisplay()
	{
		setNextButton(XSTR.getString("installButtonName"), XSTR.getString("installButtonTooltip"));
		
		Map<String, Object> settings = configuration.getSettings();
		@SuppressWarnings("unchecked")
		List<InstallerNode> modNodes = (List<InstallerNode>) settings.get(Configuration.MOD_NODES_KEY);
		@SuppressWarnings("unchecked")
		List<InstallerNode> automaticNodes = (List<InstallerNode>) settings.get(Configuration.AUTOMATIC_NODES_KEY);
		
		// do a one-time generation of these nodes
		if (automaticNodes == null)
		{
			automaticNodes = InstallerUtils.maybeCreateAutomaticNodes();
			settings.put(Configuration.AUTOMATIC_NODES_KEY, automaticNodes);
			
			// if they exist, add them to the mod nodes list
			if (!automaticNodes.isEmpty())
				modNodes.addAll(0, automaticNodes);
		}
		
		// node walks
		if (modNodeTreeWalk == null)
			modNodeTreeWalk = InstallerUtils.generateTreeWalk(modNodes);
		if (automaticNodeTreeWalk == null)
			automaticNodeTreeWalk = InstallerUtils.generateTreeWalk(automaticNodes);
		
		// populating the mod panel only needs to be done once
		if (!inited)
		{
			if (modNodes.isEmpty())
			{
				logger.error("There are no mods available!  (And this should have been checked already!)");
				return;
			}
			
			// this is only done once, and is thread-safe since it goes on the event-dispatching thread
			counter = new SharedCounter(nextButton);
			
			// populate the mod panel
			modPanel.removeAll();
			boolean shaded = false;
			Color shadedBackground = new HSLColor(getBackground()).adjustShade(5);
			for (InstallerNode node: modNodes)
			{
				addTreePanels(node, 0, shaded ? shadedBackground : null);
				shaded = !shaded;
			}
			modPanel.add(Box.createVerticalGlue());
			
			// adjust mod scroll speed to be one item per tick (based on an idea by jg18)
			if (!modNodeTreeWalk.isEmpty())
			{
				double height = ((SingleModPanel) modNodeTreeWalk.get(0).getUserObject()).getPreferredSize().getHeight();
				modScrollPane.getVerticalScrollBar().setUnitIncrement((int) height);
			}
			
			inited = true;
		}
		
		// select applicable nodes
		InstallChoice choice = (InstallChoice) settings.get(Configuration.INSTALL_CHOICE_KEY);
		if (choice == InstallChoice.BASIC)
		{
			@SuppressWarnings("unchecked")
			List<String> basicMods = (List<String>) settings.get(Configuration.BASIC_CONFIG_MODS_KEY);
			
			for (InstallerNode node: modNodeTreeWalk)
			{
				if (basicMods.contains(node.getTreePath()) && MiscUtils.validForOS(node.getName()))
				{
					logger.debug("Selecting '" + node.getTreePath() + "' as a BASIC mod");
					((SingleModPanel) node.getUserObject()).setSelected(true);
				}
				else
					((SingleModPanel) node.getUserObject()).setSelected(false);
			}
		}
		else if (choice == InstallChoice.COMPLETE)
		{
			for (InstallerNode node: modNodeTreeWalk)
			{
				logger.debug("Selecting '" + node.getTreePath() + "' as a COMPLETE mod");
				((SingleModPanel) node.getUserObject()).setSelected(MiscUtils.validForOS(node.getName()));
			}
		}
		
		// force-select certain nodes
		for (InstallerNode node: modNodeTreeWalk)
		{
			boolean force = false;
			
			// nodes that are automatically generated
			if (automaticNodeTreeWalk.contains(node))
			{
				logger.debug("Force-selecting '" + node.getTreePath() + "' as an installer-generated node");
				force = true;
			}
			// nodes where a current or previous version has been installed
			else if (configuration.getUserProperties().containsKey(node.getTreePath()))
			{
				// folder exists, which implies we installed this mod in the past and it is still valid, so select it
				if (IOUtils.isRootFolderName(node.getFolder()) || (new File(Configuration.getInstance().getApplicationDir(), node.getFolder()).exists()))
				{
					logger.debug("Force-selecting '" + node.getTreePath() + "' as already installed based on version");
					force = true;
				}
				// folder does not exist, so remove this mod from the stored configuration
				else
				{
					logger.debug("Mod '" + node.getTreePath() + "' is no longer installed; removing stored version");
					configuration.getUserProperties().remove(node.getTreePath());
				}
			}
			
			if (force)
			{
				((SingleModPanel) node.getUserObject()).setSelected(true);
				((SingleModPanel) node.getUserObject()).setAlreadyInstalled(true);
			}
		}
		
		// set Install button status
		counter.syncButton();
	}
	
	/**
	 * Instead of nesting the SingleModPanels inside of each other and returning
	 * one panel with all its children embedded, this function just stacks all
	 * the panels directly onto the modPanel.
	 */
	private void addTreePanels(InstallerNode node, int depth, Color backgroundColor)
	{
		SingleModPanel panel = new SingleModPanel(gui, node, depth, backgroundColor, counter);
		node.setUserObject(panel);
		
		modPanel.add(panel);
		for (InstallerNode child: node.getChildren())
			addTreePanels(child, depth + 1, backgroundColor);
	}
	
	@Override
	public void prepareToLeavePage(Runnable runWhenReady)
	{
		// store the mod names we selected
		Set<String> selectedMods = new LinkedHashSet<String>();
		for (InstallerNode node: modNodeTreeWalk)
			if (((SingleModPanel) node.getUserObject()).isSelected())
				selectedMods.add(node.getTreePath());
		
		// save in configuration
		Map<String, Object> settings = configuration.getSettings();
		settings.put(Configuration.MODS_TO_INSTALL_KEY, selectedMods);
		
		// also save the "force"/"re-run" checkbox
		settings.put(Configuration.DONT_SHORT_CIRCUIT_INSTALLATION_KEY, reRunCheckBox.isSelected());
		
		resetNextButton();
		runWhenReady.run();
	}
	
	private static class SingleModPanel extends JPanel
	{
		private final InstallerNode node;
		private final JCheckBox checkBox;
		private final JButton button;
		private final SharedCounter counter;
		
		public SingleModPanel(JFrame frame, InstallerNode node, int depth, Color backgroundColor, SharedCounter counter)
		{
			this.node = node;
			this.checkBox = createCheckBox(node, counter);
			this.button = createMoreInfoButton(frame, node);
			this.counter = counter;
			
			// set up layout
			setLayout(new BoxLayout(this, BoxLayout.X_AXIS));
			for (int i = 0; i < depth; i++)
				add(Box.createHorizontalStrut(15));
			add(checkBox);
			add(Box.createHorizontalGlue());
			add(button);
			
			// maybe set background color
			if (backgroundColor != null)
			{
				setBackground(backgroundColor);
				checkBox.setBackground(backgroundColor);
				button.setBackground(backgroundColor);
			}
		}
		
		@SuppressWarnings("unused")
		public InstallerNode getNode()
		{
			return node;
		}
		
		public boolean isSelected()
		{
			return checkBox.isSelected();
		}
		
		public void setSelected(boolean selected)
		{
			// keep track of tally, but don't set Install button status here
			if (checkBox.isSelected() && !selected)
				counter.numChecked--;
			else if (!checkBox.isSelected() && selected)
				counter.numChecked++;
			
			checkBox.setSelected(selected);
		}
		
		@SuppressWarnings("unused")
		public boolean isAlreadyInstalled()
		{
			return !checkBox.isEnabled();
		}
		
		public void setAlreadyInstalled(boolean installed)
		{
			checkBox.setEnabled(!installed);
		}
	}
	
	private static JCheckBox createCheckBox(final InstallerNode node, final SharedCounter counter)
	{
		JCheckBox checkBox = new JCheckBox(new AbstractAction()
		{
			{
				putValue(AbstractAction.NAME, node.getName());
			}
			
			public void actionPerformed(ActionEvent e)
			{
				// we want to automatically select or deselect appropriate nodes
				setSuperTreeState(node, ((JCheckBox) e.getSource()).isSelected());
				setSubTreeState(node, ((JCheckBox) e.getSource()).isSelected());
				
				// update check tally for this box only
				if (((JCheckBox) e.getSource()).isSelected())
					counter.numChecked++;
				else
					counter.numChecked--;
				
				// set Install button status
				counter.syncButton();
				
				// changing the selection means the installation is now custom
				Configuration.getInstance().getSettings().put(Configuration.INSTALL_CHOICE_KEY, InstallChoice.CUSTOM);
			}
			
			private void setSuperTreeState(InstallerNode root, boolean selected)
			{
				// this is only if we are *selecting* a child of an unselected parent, so do nothing if we are *deselecting*
				if (!selected)
					return;
				
				// this check exists because we don't want to doubly-set the node we're on
				if (root != node)
					((SingleModPanel) root.getUserObject()).setSelected(selected);
				
				// iterate upward through the tree
				if (root.getParent() != null)
					setSuperTreeState(root.getParent(), selected);
			}
			
			private void setSubTreeState(InstallerNode root, boolean selected)
			{
				// this is only if we are *deselecting* a parent of selected children, so do nothing if we are *selecting*
				if (selected)
					return;
				
				// this check exists because we don't want to doubly-set the node we're on
				if (root != node)
					((SingleModPanel) root.getUserObject()).setSelected(selected);
				
				// iterate through the tree
				for (InstallerNode child: root.getChildren())
					setSubTreeState(child, selected);
			}
		});
		return checkBox;
	}
	
	private static JButton createMoreInfoButton(final JFrame frame, final InstallerNode node)
	{
		JButton button = new JButton(new AbstractAction()
		{
			{
				putValue(AbstractAction.NAME, XSTR.getString("moreInfoButtonName"));
				putValue(AbstractAction.SHORT_DESCRIPTION, XSTR.getString("moreInfoButtonTooltip"));
			}
			
			public void actionPerformed(ActionEvent e)
			{
				// the name is a simple bold label
				JLabel name = new JLabel(node.getName());
				name.setFont(name.getFont().deriveFont(Font.BOLD));
				
				// make a header panel with the name, and if a version is specified, add that underneath
				JComponent header;
				if (node.getVersion() != null && !node.getVersion().startsWith("UUID"))
				{
					header = new JPanel(new GridLayout(2, 1));
					((JPanel) header).add(name);
					((JPanel) header).add(new JLabel(node.getVersion()));
				}
				else
					header = name;
				
				// we want the description to have multiline capability, so we put it in a JTextPane that looks like a JLabel
				JTextPane description = new JTextPane();
				description.setBackground(null);
				description.setEditable(false);
				description.setBorder(null);
				
				// manually wrap the description :-/
				FontMetrics metrics = description.getFontMetrics(description.getFont());
				int maxWidth = (int) (frame.getSize().getWidth() * 0.8);
				description.setText(MiscUtils.wrapText(node.getDescription(), metrics, maxWidth));
				
				// put together the panel with the header plus the description
				JPanel message = new JPanel(new BorderLayout(0, GUIConstants.DEFAULT_MARGIN));
				message.add(header, BorderLayout.NORTH);
				message.add(description, BorderLayout.CENTER);
				
				JOptionPane.showMessageDialog(frame, message, Configuration.getInstance().getApplicationTitle(), JOptionPane.INFORMATION_MESSAGE);
			}
		});
		
		if (node.getDescription() == null || node.getDescription().length() == 0)
			button.setEnabled(false);
		
		return button;
	}
	
	private static class SharedCounter
	{
		public int numChecked;
		private final JButton nextButton;
		
		public SharedCounter(JButton nextButton)
		{
			this.numChecked = 0;
			this.nextButton = nextButton;
		}
		
		public void syncButton()
		{
			nextButton.setEnabled(numChecked > 0);
		}
	}
}
