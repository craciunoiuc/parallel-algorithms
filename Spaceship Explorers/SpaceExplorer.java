import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashSet;
import java.util.Set;
import java.nio.charset.StandardCharsets;

/**
 * Class for a space explorer.
 * @author Cezar Craciunoiu 334CA
 */
public class SpaceExplorer extends Thread {

	Set<Integer> discovered;
	Integer hashCount;
	CommunicationChannel channel;
	/**
	 * Creates a {@code SpaceExplorer} object.
	 *
     * @author Cezar Craciunoiu
	 * @param hashCount
	 *            number of times that a space explorer repeats the hash operation
	 *            when decoding
	 * @param discovered
	 *            set containing the IDs of the discovered solar systems
	 * @param channel
	 *            communication channel between the space explorers and the
	 *            headquarters
	 */
	public SpaceExplorer(Integer hashCount, Set<Integer> discovered,
                         CommunicationChannel channel) {
		this.discovered = discovered;
		this.hashCount  = hashCount;
		this.channel    = channel;
		this.discovered.add(-1);
	}

    /**
     * The thread function reads two messages each in a synchronised manner until an EXIT message
     * is read. For each message received the discovered Set is checked to see if the Solar System
     * is worth exploring.
     *
     * If the message is valuable the frequency is decoded and the outgoing message is constructed
     * by combining the information from the two incoming messages and the decoded frequency.
     *
     * The function runs in parallel excepting when the messages are received (to ensure that the
     * alternating sequence is not broken) and when the discovered Set is queried (to ensure that
     * one thread doesn't check for an element that is added by another thread t the same time).
     *
     * @author Cezar Craciunoiu
     */
	@Override
	public void run() {
		Message incomingMessageDiscovered;
		Message incomingMessageNew;
		synchronized (CommunicationChannel.lock) {
			incomingMessageDiscovered = channel.getMessageHeadQuarterChannel();
			incomingMessageNew        = channel.getMessageHeadQuarterChannel();
		}
		Message outgoingMessage;
		boolean newSystem;
		while (!incomingMessageDiscovered.getData().equals("EXIT")) {
			synchronized (CommunicationChannel.discoveredLock) {
				newSystem = !discovered.contains(incomingMessageNew.getCurrentSolarSystem());
				if (newSystem) {
					discovered.add(incomingMessageNew.getCurrentSolarSystem());
				}
			}
			if (newSystem) {
				String decodedMessage = encryptMultipleTimes(
				                        incomingMessageNew.getData(), hashCount);
				outgoingMessage = incomingMessageNew;
				outgoingMessage.setData(decodedMessage);
				outgoingMessage.setParentSolarSystem(
				                incomingMessageDiscovered.getCurrentSolarSystem());
				channel.putMessageSpaceExplorerChannel(outgoingMessage);
			}
			synchronized (CommunicationChannel.lock) {
				incomingMessageDiscovered = channel.getMessageHeadQuarterChannel();
				incomingMessageNew = channel.getMessageHeadQuarterChannel();
			}
			if (incomingMessageNew.getData().equals("EXIT")) {
				break;
			}
		}
	}
	
	/**
	 * Applies a hash function to a string for a given number of times (i.e.,
	 * decodes a frequency).
	 * 
	 * @param input
	 *            string to he hashed multiple times
	 * @param count
	 *            number of times that the string is hashed
	 * @return hashed string (i.e., decoded frequency)
	 */
	private String encryptMultipleTimes(String input, Integer count) {
		String hashed = input;
		for (int i = 0; i < count; ++i) {
			hashed = encryptThisString(hashed);
		}

		return hashed;
	}

	/**
	 * Applies a hash function to a string (to be used multiple times when decoding
	 * a frequency).
	 * 
	 * @param input
	 *            string to be hashed
	 * @return hashed string
	 */
	private static String encryptThisString(String input) {
		try {
			MessageDigest md = MessageDigest.getInstance("SHA-256");
			byte[] messageDigest = md.digest(input.getBytes(StandardCharsets.UTF_8));

			// convert to string
			StringBuffer hexString = new StringBuffer();
			for (int i = 0; i < messageDigest.length; i++) {
				String hex = Integer.toHexString(0xff & messageDigest[i]);
				if (hex.length() == 1)
					hexString.append('0');
				hexString.append(hex);
			}
			return hexString.toString();

		} catch (NoSuchAlgorithmException e) {
			throw new RuntimeException(e);
		}
	}
}
