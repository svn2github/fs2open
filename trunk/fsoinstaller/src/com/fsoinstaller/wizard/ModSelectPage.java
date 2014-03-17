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
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.util.ArrayList;
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
import com.fsoinstaller.utils.Logger;
import com.fsoinstaller.utils.MiscUtils;

import static com.fsoinstaller.main.ResourceBundleManager.XSTR;


public class ModSelectPage extends WizardPage
{
	private static final Logger logger = Logger.getLogger(ModSelectPage.class);
	
	private final JPanel modPanel;
	private final JScrollPane modScrollPane;
	
	private final List<InstallerNode> treeWalk;
	
	private boolean inited;
	private SharedCounter counter;
	
	public ModSelectPage()
	{
		super("mod-select");
		
		// create widgets
		modPanel = new JPanel();
		modPanel.setLayout(new BoxLayout(modPanel, BoxLayout.Y_AXIS));
		modScrollPane = new JScrollPane(modPanel, ScrollPaneConstants.VERTICAL_SCROLLBAR_ALWAYS, ScrollPaneConstants.HORIZONTAL_SCROLLBAR_AS_NEEDED);
		
		treeWalk = new ArrayList<InstallerNode>();
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
		
		JPanel panel = new JPanel(new BorderLayout(0, GUIConstants.DEFAULT_MARGIN));
		panel.setBorder(BorderFactory.createEmptyBorder(GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN, GUIConstants.DEFAULT_MARGIN));
		panel.add(labelPanel, BorderLayout.NORTH);
		panel.add(modScrollPane, BorderLayout.CENTER);
		
		return panel;
	}
	
	@Override
	public void prepareForDisplay()
	{
		setNextButton(XSTR.getString("installButtonName"), XSTR.getString("installButtonTooltip"));
		
		Map<String, Object> settings = configuration.getSettings();
		@SuppressWarnings("unchecked")
		List<InstallerNode> modNodes = (List<InstallerNode>) settings.get(Configuration.MOD_NODES_KEY);
		
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
			treeWalk.clear();
			for (InstallerNode node: modNodes)
				addTreeNode(node, 0);
			modPanel.add(Box.createVerticalGlue());
			
			// adjust mod scroll speed to be one item per tick (based on an idea by jg18)
			if (!treeWalk.isEmpty())
			{
				double height = ((SingleModPanel) treeWalk.get(0).getUserObject()).getPreferredSize().getHeight();
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
			
			for (InstallerNode node: treeWalk)
			{
				if (basicMods.contains(node.getName()) && MiscUtils.validForOS(node.getName()))
				{
					logger.debug("Selecting '" + node.getName() + "' as a BASIC mod");
					((SingleModPanel) node.getUserObject()).setSelected(true);
				}
				else
					((SingleModPanel) node.getUserObject()).setSelected(false);
			}
		}
		else if (choice == InstallChoice.COMPLETE)
		{
			for (InstallerNode node: treeWalk)
			{
				logger.debug("Selecting '" + node.getName() + "' as a COMPLETE mod");
				((SingleModPanel) node.getUserObject()).setSelected(MiscUtils.validForOS(node.getName()));
			}
		}
		
		// select nodes that have been installed already
		for (InstallerNode node: treeWalk)
		{
			// standard selection by whether a current or previous version has been installed
			String propertyName = node.buildTreeName();
			if (configuration.getUserProperties().containsKey(propertyName))
			{
				logger.debug("Selecting '" + node.getName() + "' as already installed based on version");
				((SingleModPanel) node.getUserObject()).setSelected(true);
				((SingleModPanel) node.getUserObject()).setAlreadyInstalled(true);
			}
		}
		
		// set Install button status
		counter.syncButton();
	}
	
	private void addTreeNode(InstallerNode node, int depth)
	{
		SingleModPanel panel = new SingleModPanel(gui, node, depth, counter);
		node.setUserObject(panel);
		treeWalk.add(node);
		
		modPanel.add(panel);
		for (InstallerNode child: node.getChildren())
			addTreeNode(child, depth + 1);
	}
	
	@Override
	public void prepareToLeavePage(Runnable runWhenReady)
	{
		// store the mod names we selected
		Set<String> selectedMods = new LinkedHashSet<String>();
		for (InstallerNode node: treeWalk)
			if (((SingleModPanel) node.getUserObject()).isSelected())
				selectedMods.add(node.getName());
		
		// save in configuration
		Map<String, Object> settings = configuration.getSettings();
		settings.put(Configuration.MODS_TO_INSTALL_KEY, selectedMods);
		
		resetNextButton();
		runWhenReady.run();
	}
	
	private static class SingleModPanel extends JPanel
	{
		private final InstallerNode node;
		private final JCheckBox checkBox;
		private final JButton button;
		private final SharedCounter counter;
		
		public SingleModPanel(JFrame frame, InstallerNode node, int depth, SharedCounter counter)
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
				if (node.getVersion() != null)
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
