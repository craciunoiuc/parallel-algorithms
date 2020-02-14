import java.util.ArrayList;
import java.util.HashMap;
import java.util.Stack;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;

/**
 * Class that implements the channel used by headquarters and space explorers to communicate.
 * @author Cezar Craciunoiu 334CA
 */
public class CommunicationChannel {
    public static final Object lock           = new Object();
	private static final Object HQLock        = new Object();
    public static final Object discoveredLock = new Object();
	private LinkedBlockingQueue<Message> fromHQ   = new LinkedBlockingQueue<>();
	private LinkedBlockingQueue<Message> fromSE   = new LinkedBlockingQueue<>();
	private HashMap<Long, Message[]> buffer = new HashMap<>();
	private Message auxMessageHQ = null;
	private Message auxMessageSE = null;
	private boolean exitDetected = false;
	/**
	 * Creates a {@code CommunicationChannel} object.
	 */
	public CommunicationChannel() {
	}

	/**
	 * Puts a message on the space explorer channel (i.e., where space explorers write to and 
	 * headquarters read from).
	 *
	 * @author Cezar Craciunoiu
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageSpaceExplorerChannel(Message message) {
		try {
			fromSE.put(message);
		} catch (Exception e) { }
	}

	/**
	 * Gets a message from the space explorer channel (i.e., where space explorers write to and
	 * headquarters read from).
	 *
	 * @author Cezar Craciunoiu
	 * @return message from the space explorer channel
	 */
	public Message getMessageSpaceExplorerChannel() {
		try {
			auxMessageHQ = fromSE.take();
		} catch (Exception e) { }
		return auxMessageHQ;
	}

	/**
	 * Puts a message on the headquarters channel (i.e., where headquarters write to and 
	 * space explorers read from).
	 *
	 * @author Cezar Craciunoiu
	 * @param message
	 *            message to be put on the channel
	 */
	public void putMessageHeadQuarterChannel(Message message) {
		try {
			if (!exitDetected) {
				resynchronise(message);
			}
		} catch (Exception e) { }
	}

	/**
	 * Headquarters send two messages each without using a synchronized block. There is a chance
	 * that messages get mixed up and the Space Explorers become confused.
	 * The function checks if two messages arrived from a single HQ and afterwards sends them to
	 * the queue where Space Explorers read from.
	 * The buffer HashMap contains a number of entries equal to the number of HQs. Every entry has
	 * an array of maximum 2 elements where messages are stored. When the second element is assigned,
	 * the messages can be passed on to the explorers in a synchronised manner.
	 * If a message contains END it is ignored.
	 * If a message contains EXIT the queue is emptied and filled with enough EXIT messages for
	 * every explorer. Also, the communication channel refuses new entries afterwards.
	 *
	 * @author Cezar Craciunoiu
	 * @param message
	 *            message to be resynchronised
	 */
	private void resynchronise(Message message) {
		Thread thread = Thread.currentThread();
		if (!buffer.containsKey(thread.getId())) {
			buffer.put(thread.getId(), new Message[2]);
		}
		Message[] list = buffer.get(thread.getId());

		if (list[1] != null) {
			Message msg1 = list[0];
			Message msg2 = list[1];
			list[0] = null;
			list[1] = null;
			synchronized (HQLock) {
				try {
					fromHQ.put(msg1);
					fromHQ.put(msg2);
				} catch (Exception e) { }
			}
		}
		if (message.getData().equals("EXIT") && !exitDetected) {
			synchronized (HQLock) {
				try {
					exitDetected = true;
					fromHQ = new LinkedBlockingQueue<>();
					for (int i = 0; i < 2 * buffer.size(); ++i) {
						fromHQ.put(message);
					}

				} catch (Exception e) { }
			}
		} else {
			if (message.getData().equals("END")) {
				return;
			} else {
				if (list[0] == null) {
					list[0] = message;
				} else {
					list[1] = message;
				}
			}
		}
	}

	/**
	 * Gets a message from the headquarters channel (i.e., where headquarters write to and
	 * space explorer read from).
	 *
	 * @author Cezar Craciunoiu
	 * @return message from the header quarter channel
	 */
	public Message getMessageHeadQuarterChannel() {
		try {
			auxMessageSE = fromHQ.take();
		} catch (Exception e) { }
		return auxMessageSE;
	}
}
