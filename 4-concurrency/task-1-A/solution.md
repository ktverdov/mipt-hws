���������� ����� ������� �������� ��������� ��� ���� ������� �� ����������� �������� ����������, 
�.�. ��� ������ ������������ ����� ���������� � ����������� ������.

������, ���������� ����� ���������:
-������ ����� ������������ �������� lock():
	Thread_0					Thread_1
	victim = 0
							victim = 1
							want[1] = true
						(����� ������ � ����������� ������ ����, 
						�.�. ������� ����� �������� �� ��������� - 
						want[0] = false)
	want[0] = true
(����� ������ � ����������� ������,
�.�. ������� ����� �������� �� ��������� - 
victim == 1)