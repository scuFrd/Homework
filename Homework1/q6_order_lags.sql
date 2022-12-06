SELECT Id, OrderDate, LAG(OrderDate, 1, orderdate) OVER (ORDER BY OrderDate),ROUND(julianday(OrderDate) - julianday(LAG(OrderDate, 1, OrderDate) OVER (ORDER BY OrderDate)),2)
FROM 'Order'
WHERE CustomerId = 'BLONP'
ORDER BY OrderDate ASC
LIMIT 10;
