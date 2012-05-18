package sim_mob.vis.network;

import java.awt.AlphaComposite;
import java.awt.Color;
import java.awt.Graphics2D;
import java.awt.Polygon;
import java.awt.geom.Rectangle2D;

import sim_mob.vis.controls.DrawableItem;
import sim_mob.vis.network.basic.ScaledPoint;
import sim_mob.vis.util.Utility;

/**
 * \author Zhang Shuai
 */
public class TrafficSignalCrossing implements DrawableItem{

	private float alpha = 0.5f;
	
	private ScaledPoint nearOne;
	private ScaledPoint nearTwo;
	private ScaledPoint farOne;
	private ScaledPoint farTwo;
	private int id;
	
	public TrafficSignalCrossing(ScaledPoint nearOne, ScaledPoint nearTwo, ScaledPoint farOne, ScaledPoint farTwo,int id){
		this.nearOne = nearOne;
		this.nearTwo = nearTwo;
		this.farOne = farOne;
		this.farTwo = farTwo;
		this.id = id;
	}
	
	
	public int getZOrder() {
		return DrawableItem.Z_ORDER_TSC;
	}
	
	
	public Rectangle2D getBounds() {
		final double BUFFER_CM = 10*100; //1m
		Rectangle2D res = new Rectangle2D.Double(nearOne.getUnscaledX(), nearOne.getUnscaledY(), 0, 0);
		res.add(nearTwo.getUnscaledX(), nearTwo.getUnscaledY());
		res.add(farOne.getUnscaledX(), farOne.getUnscaledY());
		res.add(farTwo.getUnscaledX(), farTwo.getUnscaledY());
		Utility.resizeRectangle(res, res.getWidth()+BUFFER_CM, res.getHeight()+BUFFER_CM);
		return res;
	}
	
	
	public ScaledPoint getNearOne() { return nearOne; }
	public ScaledPoint getNearTwo() { return nearTwo; }
	public ScaledPoint getFarOne() { return farOne; }
	public ScaledPoint getFarTwo() { return farTwo; }
	public int getId() { return id; }

	
	
	public void drawSignalCrossing(Graphics2D g, Integer light){
		
		if(light == 1)
		{
			g.setColor(Color.RED);
			draw(g);
		} else if(light == 2){
			g.setColor(Color.YELLOW);
			draw(g);
		} else if(light == 3){
			g.setColor(Color.GREEN);
			draw(g);
		} else{
			
			System.out.println("Error, No such kind of traffic light -- TrafficSignalCrossing, drawSignalCrossing()");
		}

	}
	
	
	public void draw(Graphics2D g){
	
		g.drawLine((int)nearOne.getX(), (int)nearOne.getY(), (int)nearTwo.getX(), (int)nearTwo.getY()); 
		g.drawLine((int)farOne.getX(), (int)farOne.getY(), (int)farTwo.getX(), (int)farTwo.getY()); 

		Polygon poly = new Polygon();		
		poly.addPoint((int)nearOne.getX(), (int)nearOne.getY());
		poly.addPoint((int)nearTwo.getX(), (int)nearTwo.getY());
		poly.addPoint((int)farTwo.getX(), (int)farTwo.getY());
		poly.addPoint((int)farOne.getX(), (int)farOne.getY());
        
		g.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER,alpha));
		g.fillPolygon(poly);
		g.setComposite(AlphaComposite.getInstance(AlphaComposite.SRC_OVER,1.0f));
		
	}

}
